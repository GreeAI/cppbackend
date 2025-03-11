#pragma once

#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <chrono>
#include <random>

#include "model.h"
#include "players.h"

namespace app {

namespace json = boost::json;

enum class JoinGameErrorReason {
  InvalidName,
  InvalidMap,
  InvalidToken,
  UnknownToken
};

struct JoinGameResult {
  players::Token token;
  std::string player_id;
};

class JoinGameError {
public:
  explicit JoinGameError(JoinGameErrorReason error)
      : error_(error), error_value() {
    ParseError(error_);
  }

  std::pair<std::string, std::string> ParseError(JoinGameErrorReason error);

  std::pair<std::string, std::string> GetError() const { return error_value; }

private:
  JoinGameErrorReason error_;
  std::pair<std::string, std::string> error_value;
};

class ListPlayerUseCase {
public:
  static std::string
  GetTokenPlayers(const players::Players::TokenPlayer &players) {
    json::object players_object;
    for (const auto &[_, player] : players) {
      json::object res;
      res["name"] = player->GetName();
      players_object[std::to_string(player->GetId())] = res;
    }
    return json::serialize(players_object);
  }
};

class GameStateUseCase {
public:
  GameStateUseCase(players::Players &players) : players_(players) {}

  std::string GetState(players::Token token) const {

    const model::GameSession *game_session =
        players_.FindByToken(token)->GetGameSession();
    auto players = players_.FindPlayersBySession(game_session);

    json::object player;
    player["players"] = GetPlayersForState(players);
    player["lostObjects"] = GetLootObject(game_session);
    return json::serialize(player);
  }

  static json::object GetLootObject(const model::GameSession *session) {
    json::object loots;
    for (const model::Loot &loot : session->GetLootObjects()) {
      json::object loot_decs;

      loot_decs["type"] = loot.type;
      json::array pos = {loot.pos.x, loot.pos.y};
      loot_decs["pos"] = pos;

      loots[loot.id] = loot_decs;
    }

    return loots;
  }

  static json::object GetPlayersForState(
      const std::vector<std::shared_ptr<players::Player>> &players) {
    json::object id;

    for (const auto &player : players) {
      json::object player_state;
      const model::Dog::Position pos = player->GetDog()->GetPosition();
      player_state["pos"] = {pos.operator*().x, pos.operator*().y};

      const model::Dog::Speed speed = player->GetDog()->GetSpeed();
      player_state["speed"] = {speed.operator*().x, speed.operator*().y};

      model::Direction dir = player->GetDog()->GetDirection();
      if (dir == model::Direction::NORTH) {
        player_state["dir"] = "U";
      } else if (dir == model::Direction::SOUTH) {
        player_state["dir"] = "D";
      } else if (dir == model::Direction::WEST) {
        player_state["dir"] = "L";
      } else if (dir == model::Direction::EAST) {
        player_state["dir"] = "R";
      } else {
        player_state["dir"] = "Unknown";
      }

      player_state["bag"] = GetBagItems(player->GetDog()->GetBag());

      id[std::to_string(player->GetId())] = player_state;
    }
    return id;
  }

  static std::string SetPlayerAction(std::shared_ptr<players::Player> player,
                                     std::string move_dir) {
    float dog_speed = player->GetGameSession()->GetMap()->GetDogSpeed();
    model::Direction new_dir;
    model::Dog::Speed new_speed({0, 0});

    if (move_dir == "U") {
      new_speed = model::Dog::Speed({0, -dog_speed});
      new_dir = model::Direction::NORTH;
    } else if (move_dir == "D") {
      new_speed = model::Dog::Speed({0, dog_speed});
      new_dir = model::Direction::SOUTH;
    } else if (move_dir == "L") {
      new_speed = model::Dog::Speed({-dog_speed, 0});
      new_dir = model::Direction::WEST;
    } else if (move_dir == "R") {
      new_speed = model::Dog::Speed({dog_speed, 0});
      new_dir = model::Direction::EAST;
    }

    player->GetDog()->SetSpeed(new_speed);
    player->GetDog()->SetDirection(new_dir);
    return "{}";
  }

  std::string TickTimeUseCase(double tick, model::Game &game) {
    game.UpdateGameState(tick);
    return "{}";
  }

  void GenerateLoot(model::detail::Milliseconds delta, model::Game &game);

  static json::array GetBagItems(const model::Dog::Bag& bag_items){
    json::array items;
    for(const model::Loot& loot : *bag_items){
        json::object loot_desc;
        loot_desc["id"] = loot.id;
        loot_desc["type"] = loot.type;
  
        items.push_back(loot_desc);
    }   
  
    return items;
  };

private:
  players::Players &players_;
};

class JoinGameUseCase {
public:
  JoinGameUseCase(players::Players &players, model::Game &game)
      : players_(players), game_(game) {}

  std::string Join(std::string &map_id, std::string &user_name);

  std::pair<double, double> RandomPos(const model::Map::Roads &roads) const;

  static double GetRandomInt(int first, int second) {
    std::minstd_rand generator(static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count()));

    std::uniform_int_distribution<int> distribution(first, second);

    double random_number = distribution(generator);

    return random_number;
  }

  model::PairDouble GetFirstPos(const model::Map::Roads &roads) const;

private:
  players::Players &players_;
  model::Game &game_;
  int random_id_ = 1;
};

class Aplication {
public:
  Aplication(model::Game &game, bool set_tick, bool random_spawn)
      : game_(game), players_(), join_game_(players_, game),
        game_state_(players_), set_tick_(set_tick),
        random_spawn_(random_spawn) {}

  std::string JoinGame(std::string &map_id, std::string &user_name) {
    return join_game_.Join(map_id, user_name);
  }

  std::string GetPlayersInfo() {
    return ListPlayerUseCase::GetTokenPlayers(players_.GetPlayers());
  }

  const std::shared_ptr<players::Player> FindByToken(players::Token token) {
    return players_.FindByToken(token);
  }

  std::string GetGameState(players::Token token) {
    return game_state_.GetState(token);
  }

  std::string PlayerAction(std::shared_ptr<players::Player> player,
                           std::string move_dir) {
    return GameStateUseCase::SetPlayerAction(player, move_dir);
  }

  std::string TickTime(double tick) {
    return game_state_.TickTimeUseCase(tick, game_);
  }

  const bool IsTickSet() { return set_tick_; }

  void GenerateLoot(model::detail::Milliseconds delta) {
    return game_state_.GenerateLoot(delta, game_);
  }

private:
  model::Game &game_;
  players::Players players_;
  JoinGameUseCase join_game_;
  GameStateUseCase game_state_;
  bool set_tick_;
  bool random_spawn_;
};
} // namespace app
