#include "app.h"

namespace app {

std::string JoinGameUseCase::Join(std::string &map_id, std::string &user_name) {

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
  model::Dog *dog = game_session->AddDog(
      random_id_, model::Dog::Name(user_name),
      /*RandomPos(game_.FindMap(model::Map::Id(map_id))->GetRoads())*/
      model::Dog::Position(GetFirstPos(
          game_.FindMap(model::Map::Id(map_id))->GetRoads())), // Delete commit
      model::Dog::Speed({0., 0.}), model::Direction::NORTH);

  std::pair<players::Token, std::shared_ptr<players::Player>> player =
      players_.AddPlayer(random_id_, user_name, dog, game_session);
  random_id_++;

  json::object respons_body;
  respons_body["authToken"] = *player.first;
  respons_body["playerId"] = player.second->GetId();

  return json::serialize(respons_body);
}

std::pair<double, double>
JoinGameUseCase::RandomPos(const model::Map::Roads &roads) const {
  double road_id = GetRandomInt(0, roads.size());
  const model::Road &road = roads[road_id];

  if (road.IsHorizontal()) {
    double x = GetRandomInt(road.GetStart().x, road.GetEnd().x);
    double y = road.GetStart().y;
    return {x, y};
  } else {
    double x = road.GetStart().x;
    double y = GetRandomInt(road.GetStart().y, road.GetEnd().y);
    return {x, y};
  }
}

double JoinGameUseCase::GetRandomInt(int first, int second) const {
  return first + rand() % (second - first + 1);
}

model::Dog::PairDouble
JoinGameUseCase::GetFirstPos(const model::Map::Roads &roads) const {
  const model::Point &pos = roads.begin()->GetStart();
  return {static_cast<double>(pos.x), static_cast<double>(pos.y)};
}

} // namespace app
