#pragma once
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

#include "loot_generator.h"
#include "tagged.h"

namespace model {

namespace detail {

using Milliseconds = std::chrono::milliseconds;

inline Milliseconds FromDouble(double delta) {
  return std::chrono::duration_cast<Milliseconds>(
      std::chrono::duration<double>(delta / 1000));
}

} // namespace detail

struct LootType {
  std::optional<std::string> name;
  std::optional<std::string> file;
  std::optional<std::string> type;
  std::optional<unsigned> rotation;
  std::optional<std::string> color;
  std::optional<double> scale = 0;
};

struct PairDouble {
  double x = 0;
  double y = 0;
};

struct Loot {
  std::string id;
  unsigned type;
  PairDouble pos;
};

using Dimension = int;
using Coord = Dimension;

struct Point {
  Coord x, y;
};

struct Size {
  Dimension width, height;
};

enum class Direction { NORTH, SOUTH, WEST, EAST };

struct Rectangle {
  Point position;
  Size size;
};

struct Offset {
  Dimension dx, dy;
};

class Road {
  struct HorizontalTag {
    explicit HorizontalTag() = default;
  };

  struct VerticalTag {
    explicit VerticalTag() = default;
  };

public:
  constexpr static HorizontalTag HORIZONTAL{};
  constexpr static VerticalTag VERTICAL{};

  Road(HorizontalTag, Point start, Coord end_x) noexcept
      : start_{start}, end_{end_x, start.y} {}

  Road(VerticalTag, Point start, Coord end_y) noexcept
      : start_{start}, end_{start.x, end_y} {}

  bool IsHorizontal() const noexcept { return start_.y == end_.y; }

  bool IsVertical() const noexcept { return start_.x == end_.x; }

  Point GetStart() const noexcept { return start_; }

  Point GetEnd() const noexcept { return end_; }

  bool IsInvert() const noexcept {
    if (start_.x > end_.x || start_.y > end_.y) {
      return true;
    }
    return false;
  }

private:
  Point start_;
  Point end_;
};

inline std::ostream &operator<<(std::ostream &out, const Road &road) {
  out << road.GetStart().x << ", " << road.GetStart().y << " -- "
      << road.GetEnd().x << ", " << road.GetEnd().y;
  return out;
}

class Building {
public:
  explicit Building(Rectangle bounds) noexcept : bounds_{bounds} {}

  const Rectangle &GetBounds() const noexcept { return bounds_; }

private:
  Rectangle bounds_;
};

class Office {
public:
  using Id = util::Tagged<std::string, Office>;

  Office(Id id, Point position, Offset offset) noexcept
      : id_{std::move(id)}, position_{position}, offset_{offset} {}

  const Id &GetId() const noexcept { return id_; }

  Point GetPosition() const noexcept { return position_; }

  Offset GetOffset() const noexcept { return offset_; }

private:
  Id id_;
  Point position_;
  Offset offset_;
};

class Dog {
public:
  using Name = util::Tagged<std::string, Dog>;
  using Position = util::Tagged<PairDouble, Dog>;
  using Speed = util::Tagged<PairDouble, Dog>;

  Dog(int id, Name name, Position pos, Speed speed, Direction dir) noexcept
      : id_(id), name_(name), pos_(pos), speed_(speed), dir_(dir) {}

  int GetId() const { return id_; }

  const Name &GetName() const { return name_; }

  void SetPosition(const Position &new_pos) { pos_ = new_pos; }

  const Position &GetPosition() const { return pos_; }

  void SetSpeed(const Speed &new_speed) { speed_ = new_speed; }

  const Speed &GetSpeed() const { return speed_; }

  void SetDirection(model::Direction dir) { dir_ = dir; }

  Direction GetDirection() const { return dir_; }

private:
  int id_;
  Name name_;
  Position pos_;
  Speed speed_;
  Direction dir_;
};

inline bool operator<(const PairDouble &lhs, const PairDouble &rhs) {
  return std::tuple(lhs.x, lhs.y) < std::tuple(rhs.x, rhs.x);
}

class Map {
public:
  using Id = util::Tagged<std::string, Map>;
  enum class RoadTag { VERTICAL, HORIZONTAl };
  using Roads = std::deque<Road>;
  using RoadMap = std::map<RoadTag, std::map<double, const Road &>>;
  using RoadIt = std::map<double, const Road &>::iterator;
  using ConstRoadIt = std::map<double, const Road &>::const_iterator;
  using Buildings = std::deque<Building>;
  using Offices = std::deque<Office>;
  using LootTypes = std::deque<LootType>;

  Map(Id id, std::string name) noexcept
      : id_(std::move(id)), name_(std::move(name)) {}

  const Id &GetId() const noexcept;

  const std::string &GetName() const noexcept;

  const Buildings &GetBuildings() const noexcept;

  const Roads &GetRoads() const noexcept;

  const Offices &GetOffices() const noexcept;

  void AddRoad(const Road &road);

  std::vector<const Road *> FindRoadsByCoords(const Dog::Position &pos) const;

  void AddBuilding(const Building &building);

  void AddOffice(Office office);

  void AddDogSpeed(double new_speed);

  double GetDogSpeed() const;

  void AddLootType(LootType loot_type);

  unsigned GetRandomLootType() const;
  
  static double GetRandomInt(int first, int second) {
    std::minstd_rand generator(static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count()));

    std::uniform_int_distribution<int> distribution(first, second);

    double random_number = distribution(generator);
    return random_number;
  }

  static PairDouble GetRandomPos(const model::Map::Roads& roads) {
    model::Map::RoadTag tag = model::Map::RoadTag(GetRandomInt(0, 2));
    size_t road_index = GetRandomInt(0, roads.size());
    const model::Road& road = *(roads.begin());
    double x = 0;
    double y = 0;
    if(road.IsHorizontal()){
        x = GetRandomInt(road.GetStart().x, road.GetEnd().x);
        y = road.GetStart().y;
        return {x, y};
    }
    x = road.GetStart().x;
    y = GetRandomInt(road.GetStart().y, road.GetEnd().y);
    return {x,y};
  }

private:
  using OfficeIdToIndex =
      std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

  void FindInVerticals(const Dog::Position &pos,
                       std::vector<const Road *> &roads) const;

  void FindInHorizontals(const Dog::Position &pos,
                         std::vector<const Road *> &roads) const;

  bool CheckBounds(ConstRoadIt it, const Dog::Position &pos) const;

  Id id_;
  std::string name_;
  Roads roads_;
  RoadMap road_map_;
  Buildings buildings_;
  LootTypes loot_types_;

  OfficeIdToIndex warehouse_id_to_index_;
  Offices offices_;
  double dog_speed_ = 0;
};

class GameSession {
public:
  GameSession(const Map *map) : map_(map) {}

  Dog *AddDog(int id, const Dog::Name &name, const Dog::Position &pos,
              const Dog::Speed &vel, Direction dir) {
    dogs_.emplace_back(id, name, pos, vel, dir);
    return &dogs_.back();
  }

  const Map *GetMap() const { return map_; }

  std::deque<Dog> &GetDogs() { return dogs_; }

  const std::deque<Dog> &GetDogs() const {
    return static_cast<const std::deque<Dog> &>(dogs_);
  }

  void UpdateLoot(unsigned loot_count);

  const std::vector<Loot> &GetLootObjects() const;

private:
  unsigned auto_loot_counter_ = 0;
  std::vector<Loot> loot_;
  std::deque<Dog> dogs_;
  const Map *map_;
};

class Game {
public:
  using Maps = std::deque<Map>;

  void AddMap(Map &&map);

  GameSession *AddSession(const Map::Id &map);

  GameSession *SessionIsExists(const Map::Id &id);

  void SetDefaultDogSpeed(double new_speed);

  double GetDefaultDogSpeed() const;

  const Maps &GetMaps() const noexcept;

  const Map *FindMap(const Map::Id &id) const noexcept;

  void UpdateGameState(double delta);

  void SetLootGenerator(double period, double probability);

  detail::Milliseconds GetLootGenerator() const;

  void GenerateLootSession(detail::Milliseconds delta);

private:
  void UpdateAllDogsPositions(std::deque<Dog> &dogs, const Map *map,
                              double delta);

  void UpdateDogPos(Dog &dog, const std::vector<const Road *> &roads,
                    double delta);

  bool IsInsideRoad(const PairDouble &getting_pos, const Point &start,
                    const Point &end) const;

  using MapIdHasher = util::TaggedHasher<Map::Id>;
  using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
  using SessionsByMapId =
      std::unordered_map<Map::Id, std::deque<GameSession>, MapIdHasher>;

  Maps maps_;
  SessionsByMapId map_id_to_sessions_;
  MapIdToIndex map_id_to_index_;
  double default_dog_speed_ = 1.0;
  std::optional<loot_gen::LootGenerator> loot_generator_;
};

} // namespace model