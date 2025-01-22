#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

enum class Directions{
    NORTH,
    SOUTH,
    WEST,
    EAST
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void SetDogSpeed(double speed){
        dog_speed_ = speed;
    }

    double GetDogSpeed() const{
        return dog_speed_;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    double dog_speed_ = 1;
};

class Dog {
public:
    using Position = std::pair<double, double>;
    using Speed = std::pair<double, double>;

    Dog(int id, std::string name, Position pos, Speed speed, Directions dir) noexcept
    : id_(id)
    , name_(std::move(name))
    , pos_(pos)
    , speed_(speed)
    , directions_(dir)
    {}

    const int GetId() const noexcept {
        return id_;
    }

    const std::string GetName() const noexcept {
        return name_;
    }

    const Position& GetPosition() const {
        return pos_;
    }

    const Speed& GetSpeed() const {
        return speed_;
    }

    void SetSpeed(const Speed& speed) {
        speed_ = speed;
    }

    const Directions GetDirection() const {
        return directions_;
    }

private:
    int id_;
    std::string name_;
    Position pos_;
    Speed speed_;
    Directions directions_;
};

class GameSession {
public:
    GameSession(const Map* map): map_(map) {}

    Dog* AddDog(const int id, const std::string& name, const Dog::Position& pos, const Dog::Speed& speed, const Directions dir);

    const Map* GetMap() const {
        return map_;
    }

private:
    std::vector<Dog> dogs_;
    const Map* map_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    
    void AddMap(Map map);

    const std::vector<Map>& GetMaps() const {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession* AddGameSession(const Map::Id& map);
    GameSession* ExitSession(const Map::Id& map);

    double GetDefaultDogSpeed() const{
        return default_dog_speed_;
    }

    void SetDefaultDogSpeed(double new_speed){
        default_dog_speed_ = new_speed;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using Session = std::unordered_map<Map::Id, std::vector<GameSession>, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    Session game_session_;
    double default_dog_speed_ = 1.0;
};

}  // namespace model
