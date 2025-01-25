#include "model.h"

#include <set>
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

std::vector<const Road *> Map::FindRoadsByCoords(const Dog::Position &pos) const
{
    std::vector<const Road *> result;
    FindInVerticals(pos, result);
    FindInHorizontals(pos, result);

    return result;
}

void Map::FindInVerticals(const Dog::Position& pos, std::vector<const Road*>& roads) const{
    const auto& v_roads = road_map_.at(Map::RoadTag::VERTICAL);
    ConstRoadIt it_x = v_roads.lower_bound((*pos).x);

    if(it_x != v_roads.end()){                 
        if(it_x != v_roads.begin()){
            ConstRoadIt prev_it_x = std::prev(it_x, 1);
            if(CheckBounds(prev_it_x, pos)){
                roads.push_back(&prev_it_x->second);
            }
        }

        if(CheckBounds(it_x, pos)){
            roads.push_back(&it_x->second);
        }
    } else {
        it_x = std::prev(v_roads.end(), 1);
        if(CheckBounds(it_x, pos)){
            roads.push_back(&it_x->second);
        }
    }
}

void Map::FindInHorizontals(const Dog::Position& pos, std::vector<const Road*>& roads) const{
    const auto& h_roads = road_map_.at(Map::RoadTag::HORIZONTAl);
    ConstRoadIt it_y = h_roads.lower_bound((*pos).y);

    if(it_y != h_roads.end()){                 
        if(it_y != h_roads.begin()){
            ConstRoadIt prev_it_y = std::prev(it_y, 1);
            if(CheckBounds(prev_it_y, pos)){
                roads.push_back(&prev_it_y->second);
            }
        }

        if(CheckBounds(it_y, pos)){
            roads.push_back(&it_y->second);
        }
    } else {
        it_y = std::prev(h_roads.end(), 1);
        if(CheckBounds(it_y, pos)){
            roads.push_back(&it_y->second);
        }
    }
}

bool Map::CheckBounds(ConstRoadIt it, const Dog::Position& pos) const{
    Point start = it->second.GetStart();
    Point end = it->second.GetEnd();
    if(it->second.IsInvert()){
        std::swap(start, end);
    }
    return ((start.x - 0.4 <= (*pos).x && (*pos).x <= end.x + 0.4) && 
                (start.y - 0.4 <= (*pos).y && (*pos).y <= end.y + 0.4));
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

        Dog::DogPosition pos_stop({new_x, new_y});
        Dog::Speed result_speed(speed);

        std::set<Dog::DogPosition> collisions;

        for (auto &road : map->FindRoadsByCoords(dog.GetPosition()))
        {
            Dog::DogPosition new_pos = {new_x, new_y};

            const auto start = road->GetStart();
            const auto end = road->GetEnd();

            if (CheckDogOnRoad(new_pos, *road))
            {
                dog.SetPosition(new_pos);
                return;
            }
            // Check where dog staying
            if (start.x - 0.4 >= new_x)
            {
                pos_stop.x = start.x - 0.4;
            }
            else if (new_x >= end.x + 0.4)
            {
                pos_stop.x = end.x + 0.4;
            }

            if (start.y - 0.4 >= new_y)
            {
                pos_stop.y = start.y - 0.4;
            }
            else if (new_y >= end.y + 0.4)
            {
                pos_stop.y = end.y + 0.4;
            }

            collisions.insert(pos_stop);
        }

        if (collisions.size() != 0)
        {
            pos_stop = *(std::prev(collisions.end(), 1));
            result_speed = {0, 0};
        }
        dog.SetPosition(pos_stop);
        dog.SetSpeed(result_speed);
    }
}

bool Game::CheckDogOnRoad(const Dog::DogPosition& pos, const Road &road) const
{
    const auto start = road.GetStart();
    const auto end = road.GetEnd();

    if (start.x - 0.4 <= pos.x)
    {
        if (pos.x <= end.x + 0.4)
        {
            if (start.y - 0.4 <= pos.y)
            {
                if (pos.y <= end.y + 0.4)
                {
                    return true;
                }
            }
        }
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
