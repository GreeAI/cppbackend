#pragma once

#include <boost/json.hpp>

#include "model.h"
#include "players.h"

namespace app
{

namespace json = boost::json;

enum class JoinGameErrorReason
{
    InvalidName,
    InvalidMap,
    InvalidToken,
    UnknownToken
};

struct JoinGameResult
{
    players::Token token;
    std::string player_id;
};

class JoinGameError
{
  public:
    JoinGameError(JoinGameErrorReason error) : error_(error), error_value()
    {
        ParseError(error_);
    }

    std::pair<std::string, std::string> ParseError(JoinGameErrorReason error)
    {
        if (error == JoinGameErrorReason::InvalidMap)
        {
            json::object error_code;
            error_code["code"] = "mapNotFound";
            error_code["message"] = "Map not found";
            return std::make_pair("not_found", json::serialize(error_code));
        }
        else if (error == JoinGameErrorReason::InvalidName)
        {
            json::object error_code;
            error_code["code"] = "invalidArgument";
            error_code["message"] = "Invalid name";
            return std::make_pair("bad_request", json::serialize(error_code));
        }
        else if (error == JoinGameErrorReason::InvalidToken)
        {
            json::object error_code;
            error_code["code"] = "invalidToken";
            error_code["message"] = "Authorization header is required";
            return std::make_pair("invalidToken", json::serialize(error_code));
        }
        else
        {
            json::object error_code;
            error_code["code"] = "unknownToken";
            error_code["message"] = "Player token has not been found";
            return std::make_pair("unknownToken", json::serialize(error_code));
        }
    }

    std::pair<std::string, std::string> GetError() const
    {
        return error_value;
    }

  private:
    JoinGameErrorReason error_;
    std::pair<std::string, std::string> error_value;
};

class ListPlayerUseCase
{
  public:
    static std::string GetTokenPlayers(const players::Players::TokenPlayer &players)
    {
        json::object players_object;
        for (const auto &[_, player] : players)
        {
            json::object res;
            res["name"] = player->GetName();
            players_object[std::to_string(player->GetId())] = res;
        }
        return json::serialize(players_object);
    }
};

class GameStateUseCase
{
  public:
    GameStateUseCase(players::Players &players) : players_(players)
    {
    }

    static std::string GetState(const players::Players::TokenPlayer &players)
    {
        json::object id;

        for (const auto &[_, player] : players)
        {
            json::object player_state;
            const model::Dog::Position pos = player->GetDog()->GetPosition();
            player_state["pos"] = {pos.operator*().x, pos.operator*().y};

            const std::pair<int, int> speed = player->GetDog()->GetSpeed();
            player_state["speed"] = {speed.first, speed.second};

            model::Directions dir = player->GetDog()->GetDirection();
            if (dir == model::Directions::NORTH)
            {
                player_state["dir"] = "U";
            }
            else if (dir == model::Directions::SOUTH)
            {
                player_state["dir"] = "D";
            }
            else if (dir == model::Directions::WEST)
            {
                player_state["dir"] = "L";
            }
            else if (dir == model::Directions::EAST)
            {
                player_state["dir"] = "R";
            }
            else
            {
                player_state["dir"] = "Unknown";
            }

            id[std::to_string(player->GetId())] = player_state;
        }

        json::object player;
        player["players"] = id;
        return json::serialize(player);
    }

    static std::string SetPlayerAction(std::shared_ptr<players::Player> player, std::string move_dir)
    {
        double dog_speed = player->GetGameSession()->GetMap()->GetDogSpeed();
        if (move_dir == "U")
        {
            player->GetDog()->SetSpeed({0, -dog_speed});
        }
        else if (move_dir == "D")
        {
            player->GetDog()->SetSpeed({0, dog_speed});
        }
        else if (move_dir == "L")
        {
            player->GetDog()->SetSpeed({-dog_speed, 0});
        }
        else if (move_dir == "R")
        {
            player->GetDog()->SetSpeed({dog_speed, 0});
        }
        
        return "{}";
    }

    std::string TickTime(double tick, model::Game& game) {
        game.GameState(tick);
        return "{}";
    }

  private:
    players::Players &players_;
};

class JoinGameUseCase
{
  public:
    JoinGameUseCase(players::Players &players, model::Game &game) : players_(players), game_(game)
    {
    }

    std::string Join(std::string &map_id, std::string &user_name);

    std::pair<double, double> RandomPos(const model::Map::Roads &roads) const;
    double GetRandomInt(int first, int second) const;

    model::Dog::Position GetFirstPos(const model::Map::Roads& roads);

  private:
    model::Game &game_;
    int random_id_ = 0;
    players::Players &players_;
};

class Aplication
{
  public:
    Aplication(model::Game &game) : game_(game), players_(), join_game_(players_, game), game_state_(players_)
    {
    }

    std::string JoinGame(std::string &map_id, std::string &user_name)
    {
        return join_game_.Join(map_id, user_name);
    }

    std::string GetPlayersInfo()
    {
        return ListPlayerUseCase::GetTokenPlayers(players_.GetPlayers());
    }

    const std::shared_ptr<players::Player> FindByToken(players::Token token)
    {
        return players_.FindByToken(token);
    }

    std::string GetGameState()
    {
        return GameStateUseCase::GetState(players_.GetPlayers());
    }

    std::string PlayerAction(std::shared_ptr<players::Player> player, std::string move_dir)
    {
        return GameStateUseCase::SetPlayerAction(player, move_dir);
    }

    std::string TickTime(double tick) {
        return game_state_.TickTime(tick, game_);
    }

  private:
    model::Game &game_;
    players::Players players_;
    JoinGameUseCase join_game_;
    GameStateUseCase game_state_;
};
} // namespace app
