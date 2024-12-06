#include <boost/json.hpp>
#include "json_loader.h"
#include <fstream>
#include <iostream>

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    if (!std::filesystem::exists(json_path)) {
        throw std::runtime_error("Файл не найден: " + json_path.string());
    }
    //Содержимое файла
    std::ifstream file(json_path);
    if (!file) {
        throw std::runtime_error("Не удалось открыть файл: " + json_path.string());
    }
    std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    //Парсинг строки как json
    boost::json::value parsed_json = boost::json::parse(json_str);

    model::Game game;

    const auto& maps_array = parsed_json.as_object().at("maps").as_array();
    for(const auto& item : maps_array) {
        //Загрузка карты
        const auto& map_obj = item.as_object();
        model::Map::Id id{map_obj.at("id").as_string().c_str()};
        std::string name = json::value_to<std::string>(map_obj.at("name"));
        
        model::Map map(id, name);
        //Загрузка дорог
        for(const auto& road_item : map_obj.at("roads").as_array()){
            const auto& road_obj = road_item.as_object();
            model::Point start{static_cast<int>(road_obj.at("x0").as_int64())
                         , static_cast<int>(road_obj.at("y0").as_int64())
                        };
            if(road_obj.contains("x1")) {
                model::Coord end = static_cast<int>(road_obj.at("x1").as_int64());
                model::Road road(model::Road::HORIZONTAL, start, end);
                map.AddRoad(road);
            } else {
                model::Coord end = static_cast<int>(road_obj.at("y1").as_int64());
                model::Road road(model::Road::VERTICAL, start, end);
                map.AddRoad(road);
            }
        }
        //Загрузка домов
        for(const auto& building_item : map_obj.at("buildings").as_array()){
            const auto& building_obj = building_item.as_object();
            //Point
            model::Point pos{static_cast<int>(building_obj.at("x").as_int64())
                            ,static_cast<int>(building_obj.at("y").as_int64())
                            };
            //Size
            model::Size size{static_cast<int>(building_obj.at("w").as_int64())
                            ,static_cast<int>(building_obj.at("h").as_int64())
                            };
            
            model::Building build(model::Rectangle{pos, size});
            map.AddBuilding(build);
        }
        //Загрузка офисов
        for(const auto& offices_item : map_obj.at("offices").as_array()) {
            const auto& office_obj = offices_item.as_object();
            //id
            model::Office::Id id{office_obj.at("id").as_string().c_str()};
            //Point
            model::Point pos{static_cast<int>(office_obj.at("x").as_int64())
                            ,static_cast<int>(office_obj.at("y").as_int64())
                            };
            //Offset
            model::Offset offset{static_cast<int>(office_obj.at("offsetX").as_int64())
                                ,static_cast<int>(office_obj.at("offsetY").as_int64())
                                };
            
            model::Office office(id, pos, offset);
            map.AddOffice(office);
        }
        game.AddMap(map);
    }

    return game;
}

const std::string MapIdName(const model::Game::Maps maps) {
        json::object map_object;

        for (const auto& map : maps) {
            auto id = map.GetId();
            auto name = map.GetName();

            map_object["id"] = id.operator*();
            map_object["name"] = name;
        }

        return json::serialize(map_object);
    }

    const std::string MapFullInfo(const model::Game::Maps maps) {
        json::array json_array;

        for (const auto& map : maps) {
            auto id = map.GetId();
            auto name = map.GetName();
            auto roads = map.GetRoads();
            auto buildings = map.GetBuildings();
            auto offices = map.GetOffices();

            json::object map_object;
            map_object["id"] = id.operator*();
            map_object["name"] = name;
            
            json::array roads_array = json::array();
            for (const auto& road : roads) {
                json::object road_object;
                if(road.IsHorizontal()) {
                    road_object["x0"] = road.GetStart().x;
                    road_object["y0"] = road.GetStart().y;
                    road_object["x1"] = road.GetEnd().x;
                } else if(road.IsVertical()) {
                    road_object["x0"] = road.GetStart().x;
                    road_object["y0"] = road.GetStart().y;
                    road_object["y1"] = road.GetEnd().y;
                }
                roads_array.push_back(road_object);
            }
            map_object["roads"] = roads_array;

            json::array buildings_array = json::array();
            for (const auto& building : buildings) {
                auto bounds = building.GetBounds();
                json::object building_object;
                building_object["x"] = bounds.position.x;
                building_object["y"] = bounds.position.y;
                building_object["w"] = bounds.size.width;
                building_object["h"] = bounds.size.height;
                buildings_array.push_back(building_object);
            }
            map_object["buildings"] = buildings_array;

            json::array offices_array = json::array();
            for (const auto& office : offices) {
                json::object office_object;
                office_object["id"] = office.GetId().operator*();
                office_object["x"] = office.GetPosition().x;
                office_object["y"] = office.GetPosition().y;
                office_object["offsetX"] = office.GetOffset().dx;
                office_object["offsetY"] = office.GetOffset().dy;
                offices_array.push_back(office_object);
            }
            map_object["offices"] = offices_array;

            json_array.push_back(map_object);
        }

        return json::serialize(json_array);
    }

    std::string StatusCodeProcessing(int code) {
    json::object error_code;

    if(code == 400) {
        error_code["code"] = "badRequest";
        error_code["message"] = "Bad request"; 
    } 
    else if(code == 404) {
        error_code["code"] = "mapNotFound";
        error_code["message"] = "Map not found"; 
    }
    return json::serialize(error_code);
}

}  // namespace json_loader
