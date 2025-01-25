#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

namespace model
{

const double ROAD_HALF_WIDTH = 0.4; // Half the width of road

using Dimension = int;
using Coord = Dimension;

struct Point
{
    Coord x, y;
};

struct Size
{
    Dimension width, height;
};

struct Rectangle
{
    Point position;
    Size size;
};

struct Offset
{
    Dimension dx, dy;
};

enum class Directions
{
    NORTH,
    SOUTH,
    WEST,
    EAST
};

class Road
{
    struct HorizontalTag
    {
        HorizontalTag() = default;
    };

    struct VerticalTag
    {
        VerticalTag() = default;
    };

  public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept : start_{start}, end_{end_x, start.y}
    {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept : start_{start}, end_{start.x, end_y}
    {
    }

    bool IsHorizontal() const noexcept
    {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept
    {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept
    {
        return start_;
    }

    Point GetEnd() const noexcept
    {
        return end_;
    }

    bool IsInvert() const noexcept
    {
        if (start_.x > end_.x || start_.y > end_.y)
        {
            return true;
        }
        return false;
    }

  private:
    Point start_;
    Point end_;
};

class Building
{
  public:
    explicit Building(Rectangle bounds) noexcept : bounds_{bounds}
    {
    }

    const Rectangle &GetBounds() const noexcept
    {
        return bounds_;
    }

  private:
    Rectangle bounds_;
};

class Office
{
  public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept : id_{std::move(id)}, position_{position}, offset_{offset}
    {
    }

    const Id &GetId() const noexcept
    {
        return id_;
    }

    Point GetPosition() const noexcept
    {
        return position_;
    }

    Offset GetOffset() const noexcept
    {
        return offset_;
    }

  private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Dog
{
  public:
    struct DogPosition
    {
        double x = 0;
        double y = 0;

        bool operator<(const DogPosition &other) const
        {
            return std::tie(x, y) < std::tie(other.x, other.y);
        }
    };

    using Position = util::Tagged<DogPosition, Dog>;
    using Speed = std::pair<double, double>;

    Dog(int id, std::string name, Position pos, Speed speed, Directions dir) noexcept
        : id_(id), name_(std::move(name)), pos_(pos), speed_(speed), directions_(dir)
    {
    }

    const int GetId() const noexcept
    {
        return id_;
    }

    const std::string GetName() const noexcept
    {
        return name_;
    }

    const Position &GetPosition() const
    {
        return pos_;
    }

    const Speed &GetSpeed() const
    {
        return speed_;
    }

    void SetSpeed(const Speed &speed)
    {
        speed_ = speed;
    }

    const Directions GetDirection() const
    {
        return directions_;
    }

    void SetPosition(const DogPosition &new_pos)
    {
        Dog::Position new_dog_pos{new_pos};
        pos_ = new_dog_pos;
    }

  private:
    int id_;
    std::string name_;
    Position pos_;
    Speed speed_;
    Directions directions_;
};

class Map
{
  public:
    enum class RoadTag
    {
        VERTICAL,
        HORIZONTAl
    };

    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using ConstRoadIt = std::map<double, const Road &>::const_iterator;
    using RoadMap = std::map<RoadTag, std::map<double, const Road &>>;

    Map(Id id, std::string name) noexcept : id_(std::move(id)), name_(std::move(name))
    {
    }

    const Id &GetId() const noexcept
    {
        return id_;
    }

    const std::string &GetName() const noexcept
    {
        return name_;
    }

    const Buildings &GetBuildings() const noexcept
    {
        return buildings_;
    }

    const Roads &GetRoads() const noexcept
    {
        return roads_;
    }

    const Offices &GetOffices() const noexcept
    {
        return offices_;
    }

    void AddRoad(const Road &road)
    {
        const Road &added_road = roads_.emplace_back(road);

        if (added_road.IsVertical())
        {
            road_map_[Map::RoadTag::VERTICAL].insert({road.GetStart().x, added_road});
        }
        else
        {
            road_map_[Map::RoadTag::HORIZONTAl].insert({road.GetStart().y, added_road});
        }
    }

    void AddBuilding(const Building &building)
    {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void SetDogSpeed(double speed)
    {
        dog_speed_ = speed;
    }

    double GetDogSpeed() const
    {
        return dog_speed_;
    }

    std::vector<const Road *> FindRoadsByCoords(const Dog::Position &pos) const;

    void FindInVerticals(const Dog::Position &pos, std::vector<const Road *> &roads) const;

    void FindInHorizontals(const Dog::Position &pos, std::vector<const Road *> &roads) const;

    bool CheckBounds(ConstRoadIt it, const Dog::Position &pos) const;

  private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    RoadMap road_map_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    double dog_speed_ = 0;
};

class GameSession
{
  public:
    using Dogs = std::vector<Dog>;

    GameSession(const Map *map) : map_(map)
    {
    }

    Dog *AddDog(const int id, const std::string &name, const Dog::Position &pos, const Dog::Speed &speed,
                const Directions dir);

    const Map *GetMap() const
    {
        return map_;
    }

    Dogs &GetDogs()
    {
        return dogs_;
    }

  private:
    Dogs dogs_;
    const Map *map_;
};

class Game
{
  public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const std::vector<Map> &GetMaps() const
    {
        return maps_;
    }

    const Map *FindMap(const Map::Id &id) const noexcept
    {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end())
        {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession *AddGameSession(const Map::Id &map);
    GameSession *ExitSession(const Map::Id &map);

    double GetDefaultDogSpeed() const
    {
        return default_dog_speed_;
    }

    void SetDefaultDogSpeed(double new_speed)
    {
        default_dog_speed_ = new_speed;
    }

    void GameState(double tick);

    void UpdateDogsPos(GameSession::Dogs &dogs, const Map *map, double tick);

    bool CheckDogOnRoad(const Dog::DogPosition &pos, const Road &road) const;

  private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using Session = std::unordered_map<Map::Id, std::vector<GameSession>, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    Session game_session_;
    double default_dog_speed_ = 1.0;
};

} // namespace model
