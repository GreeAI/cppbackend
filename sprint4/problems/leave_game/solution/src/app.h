#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>

#include "players.h"
#include "model_serialization.h"
#include "connection_pool.h"

namespace app {
    namespace net = boost::asio;
    namespace sys = boost::system;
    using namespace model;
    using namespace players;

    using SharedPlayer = std::shared_ptr<Player>;
    using Milliseconds = std::chrono::milliseconds;
    using Clock = std::chrono::steady_clock;
    using Strand = net::strand<net::io_context::executor_type>;
    using DatabaseManagerPtr = std::unique_ptr<db_connection::DatabaseManager>;

    class Ticker : public std::enable_shared_from_this<Ticker>
    {
    public:
        using Handler = std::function<void(std::chrono::milliseconds delta)>;

        // Функция handler будет вызываться внутри strand с интервалом period
        Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
            : strand_{strand}, period_{period}, handler_{std::move(handler)} {}

        void Start()
        {
            net::dispatch(strand_, [self = shared_from_this()] {
                self->last_tick_ = Clock::now();
                self->ScheduleTick();
            });
        }

    private:
        void ScheduleTick()
        {
            assert(strand_.running_in_this_thread());
            timer_.expires_after(period_);
            timer_.async_wait(
                [self = shared_from_this()](sys::error_code ec) { self->OnTick(ec); });
        }

        void OnTick(sys::error_code ec)
        {
            using namespace std::chrono;
            assert(strand_.running_in_this_thread());

            if (!ec) {
                auto this_tick = Clock::now();
                auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
                last_tick_ = this_tick;
                try {
                    handler_(delta);
                } catch (std::exception &ex) {
                    std::cerr << ex.what() << std::endl;
                }
                ScheduleTick();
            }
        }

        Strand strand_;
        std::chrono::milliseconds period_;
        net::steady_timer timer_{strand_};
        Handler handler_;
        std::chrono::steady_clock::time_point last_tick_;
    };

    class PlayerTimeClock{
    public:
        PlayerTimeClock() = default;

        void IncreaseTime(size_t delta);

        std::optional<Milliseconds> GetInactivityTime() const;

        void UpdateActivity(Dog::Speed new_speed);

        Milliseconds GetPlaytime() const;
    private:
        Clock::time_point log_in_time_ = Clock::now();
        Clock::time_point current_playtime = Clock::now();
        std::optional<Clock::time_point> inactivity_start_time_ = Clock::now();
        std::optional<Clock::time_point> current_inactivity_time_;
    };

    namespace json = boost::json;

    /*-----------------------------------------------JoinGameError-----------------------------------------------*/

    enum class JoinGameErrorReason
    {
        InvalidName,
        InvalidMap,
        InvalidToken,
        UnknownToken
    };

    struct JoinGameResult
    {
        Token token;
        std::string player_id;
    };

    class JoinGameError
    {
    public:
        explicit JoinGameError(JoinGameErrorReason error)
            : error_(error), error_value()
        {
            ParseError(error_);
        }

        std::pair<std::string, std::string> ParseError(JoinGameErrorReason error);

        std::pair<std::string, std::string> GetError() const { return error_value; }

    private:
        JoinGameErrorReason error_;
        std::pair<std::string, std::string> error_value;
    };

    /*-----------------------------------------------ListPlayerUseCase-----------------------------------------------*/

    class ListPlayerUseCase
    {
    public:
        static std::string GetTokenPlayers(const Players::TokenPlayer &players)
        {
            json::object players_object;
            for (const auto &[_, player] : players) {
                json::object res;
                res["name"] = player->GetName();
                players_object[std::to_string(player->GetId())] = res;
            }
            return json::serialize(players_object);
        }
    };

    /*-----------------------------------------------GameStateUseCase-----------------------------------------------*/

    class GameStateUseCase
    {
    public:
        // Специальная хэш-функция для shared_ptr
        struct SharedPlayerHash {
            std::size_t operator()(const SharedPlayer& player) const {
                return std::hash<const Player*>()(player.get());
            }
        };

        // Специальный компаратор для shared_ptr
        struct SharedPlayerEqual {
            bool operator()(const SharedPlayer& p1, const SharedPlayer& p2) const {
                return p1.get() == p2.get();
            }
        };

        using PlayerTimeClocks = std::unordered_map<const SharedPlayer, PlayerTimeClock, SharedPlayerHash, SharedPlayerEqual>;

        GameStateUseCase(Players &players, DatabaseManagerPtr&& db) : players_(players), db_manager_(std::move(db)) {}

        std::string GetState(Token token) const
        {

            const GameSession *game_session =
                players_.FindByToken(token)->GetGameSession();
            auto players = players_.FindPlayersBySession(game_session);

            json::object player;
            player["players"] = GetPlayersForState(players);
            player["lostObjects"] = GetLootObject(game_session);
            return json::serialize(player);
        }

        static json::object GetLootObject(const GameSession *session)
        {
            json::object loots;
            for (const Loot &loot : session->GetLootObjects()) {
                json::object loot_decs;

                loot_decs["type"] = loot.type;
                json::array pos = {loot.pos.x, loot.pos.y};
                loot_decs["pos"] = pos;

                loots[std::to_string(loot.id)] = loot_decs;
            }

            return loots;
        }

        static json::object
        GetPlayersForState(const std::vector<std::shared_ptr<Player>> &players)
        {
            json::object id;

            for (const auto &player : players) {
                json::object player_state;
                const Dog::Position pos = player->GetDog()->GetPosition();
                player_state["pos"] = {pos.operator*().x, pos.operator*().y};

                const Dog::Speed speed = player->GetDog()->GetSpeed();
                player_state["speed"] = {speed.operator*().x, speed.operator*().y};

                Direction dir = player->GetDog()->GetDirection();
                if (dir == Direction::NORTH) {
                    player_state["dir"] = "U";
                } else if (dir == Direction::SOUTH) {
                    player_state["dir"] = "D";
                } else if (dir == Direction::WEST) {
                    player_state["dir"] = "L";
                } else if (dir == Direction::EAST) {
                    player_state["dir"] = "R";
                } else {
                    player_state["dir"] = "Unknown";
                }

                player_state["bag"] = GetBagItems(player->GetDog()->GetBag());
                player_state["score"] = player->GetDog()->GetScore();

                id[std::to_string(player->GetId())] = player_state;
            }
            return id;
        }

        static std::string SetPlayerAction(std::shared_ptr<Player> player,
                                           std::string move_dir)
        {
            float dog_speed = player->GetGameSession()->GetMap()->GetDogSpeed();
            Direction new_dir;
            Dog::Speed new_speed({0, 0});

            if (move_dir == "U") {
                new_speed = Dog::Speed({0, -dog_speed});
                new_dir = Direction::NORTH;
            } else if (move_dir == "D") {
                new_speed = Dog::Speed({0, dog_speed});
                new_dir = Direction::SOUTH;
            } else if (move_dir == "L") {
                new_speed = Dog::Speed({-dog_speed, 0});
                new_dir = Direction::WEST;
            } else if (move_dir == "R") {
                new_speed = Dog::Speed({dog_speed, 0});
                new_dir = Direction::EAST;
            }

            player->GetDog()->SetSpeed(new_speed);
            player->GetDog()->SetDirection(new_dir);
            return "{}";
        }

        std::string TickTimeUseCase(double tick, Game &game)
        {
            std::deque<SharedPlayer> retired_players;
            for(auto& [player, clock] : clocks_){
                clock.IncreaseTime(tick);
                auto inactivity_time = clock.GetInactivityTime();
                if(inactivity_time.has_value()){
                    int converted_time_ms = static_cast<double>(inactivity_time->count());
                    if(converted_time_ms >= (game.GetDogRetirementTime() * 1000)){
                        retired_players.push_back(player);
                    }
                }
            }

            for(const SharedPlayer player : retired_players){
                SaveScore(player, game);
                DisconnectPlayer(player, game);
            }
            game.UpdateGameState(tick);
            return "{}";
        }

        void GenerateLoot(model::detail::Milliseconds delta, Game &game);

        static json::array GetBagItems(const Dog::Bag &bag_items)
        {
            json::array items;
            for (const Loot &loot : *bag_items) {
                json::object loot_desc;
                loot_desc["id"] = loot.id;
                loot_desc["type"] = loot.type;

                items.push_back(loot_desc);
            }

            return items;
        };

        std::string GetRecords(int start, int max_items);

        void AddPlayerTimeClock(Player* player);

        void SaveScore(const SharedPlayer player, Game& game);

        void DisconnectPlayer(const SharedPlayer player, Game& game);

    private:
        Players &players_;
        PlayerTimeClocks clocks_;
        DatabaseManagerPtr db_manager_;
    };

    /*-----------------------------------------------JoinGameUseCase-----------------------------------------------*/

    class JoinGameUseCase
    {
    public:
        JoinGameUseCase(Players &players, Game &game)
            : players_(players), game_(game) {}

        std::string Join(std::string &map_id, std::string &user_name, bool random_spawn);

        std::pair<double, double> RandomPos(const Map::Roads &roads) const;

        static double GetRandomInt(int first, int second)
        {
            std::minstd_rand generator(static_cast<int>(
                std::chrono::system_clock::now().time_since_epoch().count()));

            std::uniform_int_distribution<int> distribution(first, second);

            double random_number = distribution(generator);

            return random_number;
        }

        PairDouble GetFirstPos(const Map::Roads &roads) const;

    private:
        Players &players_;
        Game &game_;
        int random_id_ = 1;
    };

    /*-----------------------------------------------GameSaveCase-----------------------------------------------*/

    class GameSaveCase
    {
    public:
        GameSaveCase(std::string state_file, std::optional<int> tick,
                     Players &players, const Game::SessionsByMapId &session)
            : last_tick_(Clock::now()), state_file_(state_file),
              save_tick_(tick), sessions_(session), players_(players)
            {}

        void SaveOnTick(bool is_periodic)
        {
            if (save_tick_.has_value()) {
                if (is_periodic) {
                    Clock::time_point this_tick = Clock::now();
                    auto delta =
                        std::chrono::duration_cast<Milliseconds>(this_tick - last_tick_);
                    if (delta >= model::detail::FromDouble(save_tick_.value())) {
                        SaveState();
                        last_tick_ = Clock::now();
                    }
                } else {
                    SaveState();
                }
            }
        }

        void SaveState()
        {
            using namespace std::literals;
            std::fstream fstrm(state_file_, std::ios::out);
            boost::archive::text_oarchive output_archive{fstrm};
            serialization::GameStateRepr writed_game_state(sessions_, players_);
            output_archive << writed_game_state;
        }

        serialization::GameStateRepr LoadState()
        {
            using namespace std::literals;
            std::fstream fstrm(state_file_, std::ios::in);
            serialization::GameStateRepr game_state;
            try {
                boost::archive::text_iarchive input_archive{fstrm};
                input_archive >> game_state;
                return game_state;
            } catch (...) {
                return game_state;
            }
        }

    private:
        Clock::time_point last_tick_;
        std::string state_file_;
        std::optional<int> save_tick_;
        const Game::SessionsByMapId &sessions_;
        Players &players_;
    };

    /*-----------------------------------------------Aplication-----------------------------------------------*/

    class Aplication
    {
    public:
        Aplication(Game &game, bool set_tick,
                   std::optional<std::string> state, std::optional<int> tick_state, bool random_spawn, DatabaseManagerPtr&& db, Strand strand)
            : game_(game), players_(), join_game_(players_, game),
              game_state_(players_, std::move(db)), set_tick_(set_tick),
              random_spawn_(random_spawn), api_strand_(strand)
        {
            if (state.has_value()) {
                std::cout << state.value() << std::endl;
                save_case_.emplace(state.value(), tick_state, players_,
                                   game_.GetAllSessions());
            }
        }

        std::string JoinGame(std::string &map_id, std::string &user_name)
        {
            return join_game_.Join(map_id, user_name, random_spawn_);
        }

        std::string GetPlayersInfo()
        {
            return ListPlayerUseCase::GetTokenPlayers(players_.GetPlayers());
        }

        const SharedPlayer FindByToken(Token token)
        {
            return players_.FindByToken(token);
        }

        std::string GetGameState(Token token) { return game_state_.GetState(token); }

        std::string PlayerAction(SharedPlayer player,
                                 std::string move_dir)
        {
            return GameStateUseCase::SetPlayerAction(player, move_dir);
        }

        std::string TickTime(double tick)
        {
            std::string res = game_state_.TickTimeUseCase(tick, game_);
            if (save_case_.has_value()) {
                save_case_.value().SaveOnTick(set_tick_);
            }
            return res;
        }

        bool IsTickSet() { return set_tick_; }

        void GenerateLoot(model::detail::Milliseconds delta)
        {
            return game_state_.GenerateLoot(delta, game_);
        }

        void SaveState()
        {
            if (save_case_.has_value()) {
                save_case_.value().SaveState();
            }
        }

        Strand& GetStrand(){
            return api_strand_;
        }

        std::string GetRecords(int start, int max_items){
            return game_state_.GetRecords(start, max_items);
        }

        void LoadState()
        {
            if (save_case_.has_value()) {
                auto game_state = save_case_.value().LoadState();
                for (const auto &[map_id, sessions] : game_state.GetAllSessions()) {
                    for (const auto &session_repr : sessions) {
                        GameSession *session = game_.AddSession(Map::Id(map_id));
                        // Загрузка потерянных объектов
                        session->SetLootObjects(session_repr.GetLoot());
                        for (const auto &dog_repr : session_repr.GetDogsRepr()) {
                            // Загрузка собаки
                            Dog *created_dog = session->AddCreatedDog(dog_repr.Restore());
                            const auto &player_repr = dog_repr.GetPlayerRepr();
                            // Загрузка игрока
                            auto added_player =
                                players_.AddPlayer(player_repr.GetId(), player_repr.GetName(),
                                                   created_dog, session);

                            players_.LoadPlayerToken(added_player.second,
                                                     Token(player_repr.GetToken()));
                                                     
                            std::cout << player_repr.GetToken() << " Token in app " << std::endl;
                            players_.LoadPlayerInSession(added_player.second, session);
                        }
                    }
                }
            }
        }

        

    private:
        Game &game_;
        Players players_;
        JoinGameUseCase join_game_;
        GameStateUseCase game_state_;
        bool set_tick_;
        bool random_spawn_;
        std::optional<GameSaveCase> save_case_;
        Strand api_strand_;
        std::shared_ptr<Ticker> time_ticker_;
        std::shared_ptr<Ticker> loot_ticker_;
    };
} // namespace app
