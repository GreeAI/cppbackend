#include "model.h"

#include <stdexcept>

namespace model
{
using namespace std::literals;

void Map::AddOffice(Office office)
{
    if (warehouse_id_to_index_.contains(office.GetId()))
    {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office &o = offices_.emplace_back(std::move(office));
    try
    {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    }
    catch (...)
    {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

std::vector<const Road*> Map::FindRoadsByCoords(const Dog::Position& pos) const{
    std::vector<const Road*> result;
    FindInVerticals(pos, result);
    FindInHorizontals(pos, result);

    return result;
}

void Map::FindInVerticals(const Dog::Position &pos, std::vector<const Road *> &roads) const
{
    double dog_x = pos.operator*().x;

    for (const auto &road : roads_) {
        if (road.IsHorizontal()) {
            double road_x = road.GetStart().x;
            
            if (dog_x >= (road_x - ROAD_HALF_WIDTH) && dog_x <= (road_x + ROAD_HALF_WIDTH)) {
                roads.push_back(&road);
            }
        }
    }
}

void Map::FindInHorizontals(const Dog::Position &pos, std::vector<const Road *> &roads) const
{
    double dog_y = pos.operator*().y;

    for (const auto &road : roads_) {
        if (road.IsHorizontal()) {
            double road_y = road.GetStart().y;

            if (dog_y >= (road_y - ROAD_HALF_WIDTH) && dog_y <= (road_y + ROAD_HALF_WIDTH)) {
                roads.push_back(&road);
            }
        }
    }
}

bool Map::CheckBounds(const Road &road, const Dog::Position &pos) const
{
    Point start = road.GetStart();
    Point end = road.GetEnd();

    if (start.x - ROAD_HALF_WIDTH <= (*pos).x && (*pos).x <= end.x + ROAD_HALF_WIDTH)
    {
        return true;
    }
    else if (start.y - ROAD_HALF_WIDTH <= (*pos).y && (*pos).x <= end.y + ROAD_HALF_WIDTH)
    {
        return true;
    }
    return false;
}

void Game::AddMap(Map map)
{
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted)
    {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    }
    else
    {
        try
        {
            maps_.emplace_back(std::move(map));
        }
        catch (...)
        {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession *Game::AddGameSession(const Map::Id &map)
{
    const Map *map_id = FindMap(map);
    if (FindMap(map) != nullptr)
    {
        GameSession *game_session = &(game_session_[map].emplace_back(map_id));
        return game_session;
    }
    return nullptr;
}

GameSession *Game::ExitSession(const Map::Id &map)
{
    const Map *map_id = FindMap(map);
    if (FindMap(map) != nullptr)
    {
        GameSession *game_session = &(game_session_[map].emplace_back(map_id));
        return game_session;
    }
    return nullptr;
}

void Game::GameState(double tick)
{
    for (auto &[id, game_session] : game_session_)
    {
        for (auto &session : game_session)
        {
            UpdateDogsPos(session.GetDogs(), session.GetMap(), tick);
        }
    }
}

void Game::UpdateDogsPos(GameSession::Dogs &dogs, const Map *map, double tick)
{
    for (auto &dog : dogs)
    {
        auto position = dog.GetPosition();
        auto speed = dog.GetSpeed();

        double new_x = position.operator*().x + speed.first * tick;
        double new_y = position.operator*().y + speed.second * tick;
        for (auto &road : map->FindRoadsByCoords(dog.GetPosition()))
        {
            Dog::DogPosition new_pos = {new_x, new_y};

            if (CheckDogOnRoad(new_pos, *road))
            {
                dog.SetPosition(new_pos);
                return;
            }
            // Check where dog staying
            if (road->IsHorizontal())
            {
                double upper_bound = road->GetStart().y + ROAD_HALF_WIDTH;
                double lower_bound = road->GetStart().y - ROAD_HALF_WIDTH;
                if (new_y > upper_bound)
                {
                    new_y = upper_bound;
                }
                else if (new_y < lower_bound)
                {
                    new_y = lower_bound;
                }
            }
            else if (road->IsVertical())
            {
                // Находим границу по x
                double right_bound = road->GetStart().x + ROAD_HALF_WIDTH;
                double left_bound = road->GetStart().x - ROAD_HALF_WIDTH;
                if (new_x > right_bound)
                {
                    new_x = right_bound;
                }
                else if (new_x < left_bound)
                {
                    new_x = left_bound;
                }
            }
        }
        Dog::DogPosition pos_stop = {new_x, new_y};
        dog.SetPosition(pos_stop);
        dog.SetSpeed({0, 0});
    }
}

bool Game::CheckDogOnRoad(const Dog::DogPosition pos, const Road &road) const
{
    const auto start = road.GetStart();
    const auto end = road.GetEnd();

    if (road.IsHorizontal())
    {
        // Проверяем верхнюю и нижнюю границы дороги
        double upper_bound = start.y + ROAD_HALF_WIDTH;
        double lower_bound = start.y - ROAD_HALF_WIDTH;
        return (pos.y >= lower_bound && pos.y <= upper_bound) && (pos.x >= start.x && pos.x <= end.x);
    }
    else if (road.IsVertical())
    {
        // Проверяем левую и правую границы дороги
        double right_bound = start.x + ROAD_HALF_WIDTH;
        double left_bound = start.x - ROAD_HALF_WIDTH;
        return (pos.x >= left_bound && pos.x <= right_bound) && (pos.y >= start.y && pos.y <= end.y);
    }

    return false;
}

Dog *GameSession::AddDog(const int id, const std::string &name, const Dog::Position &pos, const Dog::Speed &speed,
                         const Directions dir)
{
    dogs_.emplace_back(id, name, pos, speed, dir);
    return &dogs_.back();
}

} // namespace model
