#include "model.h"

#include <exception>
#include <set>
#include <stdexcept>

const double HALF_ROAD = 0.4;

namespace model {
using namespace std::literals;

/* ------------------------ Map ----------------------------------- */

const Map::Id &Map::GetId() const noexcept { return id_; }

const std::string &Map::GetName() const noexcept { return name_; }

const Map::Buildings &Map::GetBuildings() const noexcept { return buildings_; }

const Map::Roads &Map::GetRoads() const noexcept { return roads_; }

const Map::Offices &Map::GetOffices() const noexcept { return offices_; }

void Map::AddRoad(const Road &road) {
  const Road &added_road = roads_.emplace_back(road);

  if (added_road.IsVertical()) {
    road_map_[Map::RoadTag::VERTICAL].insert({road.GetStart().x, added_road});
  } else {
    road_map_[Map::RoadTag::HORIZONTAl].insert({road.GetStart().y, added_road});
  }
}

std::vector<const Road *>
Map::FindRoadsByCoords(const Dog::Position &pos) const {
  std::vector<const Road *> result;
  FindInVerticals(pos, result);
  FindInHorizontals(pos, result);

  return result;
}

void Map::AddBuilding(const Building &building) {
  buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
  if (warehouse_id_to_index_.contains(office.GetId())) {
    throw std::invalid_argument("Duplicate warehouse");
  }

  const size_t index = offices_.size();
  Office &o = offices_.emplace_back(std::move(office));
  try {
    warehouse_id_to_index_.emplace(o.GetId(), index);
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    // Удаляем офис из вектора, если не удалось вставить в unordered_map
    offices_.pop_back();
    throw;
  }
}

void Map::AddDogSpeed(double new_speed) { dog_speed_ = new_speed; }

double Map::GetDogSpeed() const { return dog_speed_; }

void Map::FindInVerticals(const Dog::Position &pos,
                          std::vector<const Road *> &roads) const {
  const auto &v_roads = road_map_.at(Map::RoadTag::VERTICAL);
  ConstRoadIt it_x = v_roads.lower_bound(
      (*pos).x); /* Ищем ближайшую дорогу по полученной координате */

  if (it_x != v_roads.end()) {
    if (it_x != v_roads.begin()) { /* Если ближайшая дорога не является первой,
                                      то нужно проверить предыдущую */
      ConstRoadIt prev_it_x = std::prev(it_x, 1);
      if (CheckBounds(prev_it_x, pos)) {
        roads.push_back(&prev_it_x->second);
      }
    }

    if (CheckBounds(it_x, pos)) { /* Проверяем ближайшую дорогу */
      roads.push_back(&it_x->second);
    }
  } else { /* Если дорога не найдена, то полученная координата дальше всех
              дорог*/
    it_x = std::prev(v_roads.end(), 1); /* Тогда проверяем последнюю дорогу */
    if (CheckBounds(it_x, pos)) {
      roads.push_back(&it_x->second);
    }
  }
}

void Map::FindInHorizontals(const Dog::Position &pos,
                            std::vector<const Road *> &roads) const {
  const auto &h_roads = road_map_.at(Map::RoadTag::HORIZONTAl);
  ConstRoadIt it_y = h_roads.lower_bound(
      (*pos).y); /* Ищем ближайшую дорогу по полученной координате */

  if (it_y != h_roads.end()) {
    if (it_y != h_roads.begin()) { /* Если ближайшая дорога не является первой,
                                      то нужно проверить предыдущую */
      ConstRoadIt prev_it_y = std::prev(it_y, 1);
      if (CheckBounds(prev_it_y, pos)) {
        roads.push_back(&prev_it_y->second);
      }
    }

    if (CheckBounds(it_y, pos)) { /* Проверяем ближайшую дорогу */
      roads.push_back(&it_y->second);
    }
  } else { /* Если дорога не найдена, то полученная координата дальше всех
              дорог*/
    it_y = std::prev(h_roads.end(), 1); /* Тогда проверяем последнюю дорогу */
    if (CheckBounds(it_y, pos)) {
      roads.push_back(&it_y->second);
    }
  }
}

bool Map::CheckBounds(ConstRoadIt it, const Dog::Position &pos) const {
  Point start = it->second.GetStart();
  Point end = it->second.GetEnd();
  if (it->second.IsInvert()) {
    std::swap(start, end);
  }
  return ((start.x - HALF_ROAD <= (*pos).x && (*pos).x <= end.x + HALF_ROAD) &&
          (start.y - HALF_ROAD <= (*pos).y && (*pos).y <= end.y + HALF_ROAD));
}

void Map::AddLootType(LootType loot_type) {
  loot_types_.emplace_back(std::move(loot_type));
}

unsigned Map::GetRandomLootType() const {
  return GetRandomInt(0, loot_types_.size());
}


/* ------------------------ GameSession ---------------------------- */

void GameSession::UpdateLoot(unsigned loot_count){
  for(unsigned i = 0; i < loot_count; ++i){
      unsigned type = map_->GetRandomLootType();
      const model::Map::Roads& roads = map_->GetRoads();
      PairDouble pos = Map::GetRandomPos(map_->GetRoads());

      loot_.emplace_back(std::to_string(auto_loot_counter_++), type, pos);
  }
}

const std::vector<Loot>& GameSession::GetLootObjects() const{
  return loot_;
}

/* ------------------------ Game ----------------------------------- */

void Game::AddMap(Map &&map) {
  const size_t index = maps_.size();
  if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index);
      !inserted) {
    throw std::invalid_argument("Map with id "s + *map.GetId() +
                                " already exists"s);
  } else {
    try {
      maps_.emplace_back(std::move(map));
    } catch (std::exception &ex) {
      std::cerr << ex.what() << std::endl;
      map_id_to_index_.erase(it);
      throw;
    }
  }
}

GameSession *Game::AddSession(const Map::Id &id) {
  if (const Map *map = FindMap(id); map != nullptr) {
    GameSession *session = &(map_id_to_sessions_[id].emplace_back(map));
    return session;
  }
  return nullptr;
}

GameSession *Game::SessionIsExists(const Map::Id &id) {
  if (const Map *map = FindMap(id); map != nullptr) {
    if (!map_id_to_sessions_[id].empty()) {
      GameSession *session = &(map_id_to_sessions_[id].back());
      return session;
    }
  }
  return nullptr;
}

void Game::SetDefaultDogSpeed(double new_speed) {
  default_dog_speed_ = new_speed;
}

double Game::GetDefaultDogSpeed() const { return default_dog_speed_; }

const Game::Maps &Game::GetMaps() const noexcept { return maps_; }

const Map *Game::FindMap(const Map::Id &id) const noexcept {
  if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
    return &maps_.at(it->second);
  }
  return nullptr;
}

void Game::UpdateGameState(double delta) {
  for (auto &[map_id, sessions] : map_id_to_sessions_) {
    for (GameSession &session : sessions) {
      UpdateAllDogsPositions(session.GetDogs(), session.GetMap(), delta);
    }
  }
}

void Game::UpdateAllDogsPositions(std::deque<Dog> &dogs, const Map *map,
                                  double delta) {
  for (Dog &dog : dogs) {
    std::vector<const Road *> roads = map->FindRoadsByCoords(dog.GetPosition());
    UpdateDogPos(dog, roads, delta);
  }
}

void Game::UpdateDogPos(Dog &dog, const std::vector<const Road *> &roads,
                        double delta) {
  const auto [x, y] = *(dog.GetPosition());
  const auto [vx, vy] = *(dog.GetSpeed());

  const PairDouble getting_pos({x + vx * delta, y + vy * delta});
  const PairDouble getting_speed({vx, vy});

  PairDouble result_pos(getting_pos);
  PairDouble result_speed(getting_speed);

  std::set<PairDouble> collisions;

  for (const Road *road : roads) {
    Point start = road->GetStart();
    Point end = road->GetEnd();

    if (road->IsInvert()) {
      std::swap(start, end);
    }

    if (IsInsideRoad(getting_pos, start, end)) {
      dog.SetPosition(Dog::Position(getting_pos));
      dog.SetSpeed(Dog::Speed(getting_speed));
      return;
    }

    if (start.x - HALF_ROAD >= getting_pos.x) {
      result_pos.x = start.x - HALF_ROAD;
    } else if (getting_pos.x >= end.x + HALF_ROAD) {
      result_pos.x = end.x + HALF_ROAD;
    }

    if (start.y - HALF_ROAD >= getting_pos.y) {
      result_pos.y = start.y - HALF_ROAD;
    } else if (getting_pos.y >= end.y + HALF_ROAD) {
      result_pos.y = end.y + HALF_ROAD;
    }

    collisions.insert(result_pos);
  }

  if (collisions.size() != 0) {
    result_pos = *(std::prev(collisions.end(), 1));
    result_speed = {0, 0};
  }

  dog.SetPosition(Dog::Position(result_pos));
  dog.SetSpeed(Dog::Speed(result_speed));
}

bool Game::IsInsideRoad(const PairDouble &getting_pos, const Point &start,
                        const Point &end) const {
  if (start.x - HALF_ROAD <= getting_pos.x) {
    if (getting_pos.x <= end.x + HALF_ROAD) {
      if (start.y - HALF_ROAD <= getting_pos.y) {
        if (getting_pos.y <= end.y + HALF_ROAD) {
          return true;
        }
      }
    }
  }

  return false;
}

void Game::SetLootGenerator(double period, double probability){
  loot_generator_.emplace(detail::FromDouble(period), probability);
}

detail::Milliseconds Game::GetLootGenerator() const {
  return loot_generator_.value().GetInterval();
}

void Game::GenerateLootSession(detail::Milliseconds delta) {
  try{
    for(auto& [map_id, sessions] : map_id_to_sessions_){
      for(GameSession& session : sessions){
          unsigned current_loot_count = session.GetLootObjects().size();
          unsigned loot_count = (*loot_generator_).Generate(delta, current_loot_count, session.GetDogs().size());
          std::cout << "UpdateLoot\n";
          session.UpdateLoot(loot_count);
      }
    }
  }
  catch(std::exception ex) {
    std::cerr << ex.what() << std::endl;
  } 
  
}

} // namespace model