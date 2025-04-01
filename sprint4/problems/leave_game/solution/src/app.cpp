#include "app.h"

namespace app {

    void PlayerTimeClock::IncreaseTime(size_t delta){
    current_playtime += Milliseconds(delta);

    if(inactivity_start_time_){
        if(current_inactivity_time_){
            current_inactivity_time_ = current_inactivity_time_.value() + Milliseconds(delta);
        } else {
            current_inactivity_time_ = inactivity_start_time_.value() + Milliseconds(delta);
        }
    }
}

std::optional<Milliseconds> PlayerTimeClock::GetInactivityTime() const{
    if(current_inactivity_time_ && inactivity_start_time_){
        return duration_cast<Milliseconds>(current_inactivity_time_.value() - inactivity_start_time_.value());
    } else {
        return std::nullopt;
    }
}

void PlayerTimeClock::UpdateActivity(Dog::Speed new_speed){
    if(inactivity_start_time_.has_value() && new_speed != Dog::Speed({0, 0})){
        inactivity_start_time_ = std::nullopt;
        current_inactivity_time_ = std::nullopt;
    } else if(!inactivity_start_time_.has_value() && new_speed == Dog::Speed({0, 0})){
        inactivity_start_time_ = Clock::now();
    } 
}

Milliseconds PlayerTimeClock::GetPlaytime() const{
    return duration_cast<Milliseconds>(current_playtime - log_in_time_);
}

/*----------------------------------------JoinGameError----------------------------------------*/

    std::pair<std::string, std::string>
    JoinGameError::ParseError(JoinGameErrorReason error)
    {
        if (error == JoinGameErrorReason::InvalidMap) {
            json::object error_code;
            error_code["code"] = "mapNotFound";
            error_code["message"] = "Map not found";
            return std::make_pair("not_found", json::serialize(error_code));
        }

        if (error == JoinGameErrorReason::InvalidName) {
            json::object error_code;
            error_code["code"] = "invalidArgument";
            error_code["message"] = "Invalid name";
            return std::make_pair("bad_request", json::serialize(error_code));
        }

        if (error == JoinGameErrorReason::InvalidToken) {
            json::object error_code;
            error_code["code"] = "invalidToken";
            error_code["message"] = "Authorization header is required";
            return std::make_pair("invalidToken", json::serialize(error_code));
        }

        json::object error_code;
        error_code["code"] = "unknownToken";
        error_code["message"] = "Player token has not been found";
        return std::make_pair("unknownToken", json::serialize(error_code));
    }

/*----------------------------------------GameStateUseCase----------------------------------------*/

    std::string GameStateUseCase::Join(std::string &map_id, std::string &user_name, bool random_spawn)
    {

        if (game_.FindMap(model::Map::Id(map_id)) == nullptr) {
            throw JoinGameError(JoinGameErrorReason::InvalidMap);
        }

        if (user_name.empty()) {
            throw JoinGameError(JoinGameErrorReason::InvalidName);
        }

        model::GameSession *game_session =
            game_.SessionIsExists(model::Map::Id(map_id));

        if (game_session == nullptr) {
            game_session = game_.AddSession(model::Map::Id(map_id));
        }

        Dog::Name dog_name(user_name);
        Dog::Position dog_pos = (random_spawn) 
            ? Dog::Position(Map::GetRandomPos(game_.FindMap(model::Map::Id(map_id))->GetRoads())) 
            : Dog::Position(Map::GetFirstPos(game_.FindMap(model::Map::Id(map_id))->GetRoads()));
        Dog::Speed dog_speed({0, 0});
        Direction dog_dir = Direction::NORTH;

        Dog* dog = game_session->AddDog(random_id_, dog_name, dog_pos, 
                                        dog_speed, dog_dir);

        game_session->UpdateLoot(game_session->GetDogs().size() - game_session->GetLootObjects().size());

        std::pair<players::Token, SharedPlayer> player =
            players_.AddPlayer(random_id_, user_name, dog, game_session);
        random_id_++;

        AddPlayerTimeClock(player.second);

        json::object respons_body;
        respons_body["authToken"] = *player.first;
        respons_body["playerId"] = player.second->GetId();

        return json::serialize(respons_body);
    }

    std::pair<double, double>
    GameStateUseCase::RandomPos(const model::Map::Roads &roads) const
    {
        double road_id = GetRandomInt(0, roads.size());
        const model::Road &road = roads[road_id];

        if (road.IsHorizontal()) {
            double x = GetRandomInt(road.GetStart().x, road.GetEnd().x);
            double y = road.GetStart().y;
            return {x, y};
        }
        double x = road.GetStart().x;
        double y = GetRandomInt(road.GetStart().y, road.GetEnd().y);
        return {x, y};
    }

    model::PairDouble
    GameStateUseCase::GetFirstPos(const model::Map::Roads &roads) const
    {
        const model::Point &pos = roads.begin()->GetStart();
        return {static_cast<double>(pos.x), static_cast<double>(pos.y)};
    }

    void GameStateUseCase::GenerateLoot(model::detail::Milliseconds delta, model::Game &game)
    {

        game.GenerateLootInSessions(delta);
    }

    std::string GameStateUseCase::GetRecords(int start, int max_items) {
        json::array records;

        auto res = db_manager_->SelectData(start, max_items);
        for(const auto& [name, score, time] : res.iter<std::string, int, double>()){
            json::object entry;
            entry["name"] = name;
            entry["playTime"] = time;
            entry["score"] = score;
            records.push_back(entry);
        }

        return json::serialize(records);
    }

    void GameStateUseCase::AddPlayerTimeClock(SharedPlayer player) {
       auto emplace_result = clocks_.emplace(player, PlayerTimeClock());
        if(emplace_result.second){
            PlayerTimeClock& clock = emplace_result.first->second;
            emplace_result.first->first->SetPlayerTimeClock([&clock](Dog::Speed new_speed){
                clock.UpdateActivity(new_speed);
            });        
        }  
    }

    void GameStateUseCase::SaveScore(const SharedPlayer player, Game& game) {
        std::string name = player->GetName();
        int score = player->GetDog()->GetScore();
        double given_time = static_cast<double>(clocks_.at(player).GetPlaytime().count()) / 1000;
        double time = std::min(given_time, static_cast<double>(game.GetDogRetirementTime()));
        
        db_manager_->InsertData(name, score, time);
    }

    void GameStateUseCase::DisconnectPlayer(const SharedPlayer player, Game& game) {
        const GameSession* player_game_session = player->GetGameSession();
        const Dog* player_dog =  player->GetDog();

        auto it = clocks_.find(player);
        clocks_.erase(it);
        players_.DeletePlayer(player);

        game.DisconnectDogFromSession(player_game_session, player_dog);
    }

} // namespace app
