#pragma once

#include <boost/json.hpp>
#include <filesystem>

#include "model.h"

namespace json = boost::json;

namespace json_loader {

model::Game LoadGame(const std::filesystem::path &json_path);
void LoadingRoadIntoMap(model::Map &map, const boost::json::array &roads_array);
void LoadingBuildingsIntoMap(model::Map &map,
                             const boost::json::array &buildings_array);
void LoadingOfficesIntoMap(model::Map &map,
                           const boost::json::array &offices_array);
void AddLootTypesFromJson(const json::object& json_map, model::Map& map);

const std::string MapIdName(const model::Game::Maps &maps);
const std::string MapFullInfo(const model::Map &map);
// std::string StatusCodeProcessing(int code);
// std::string GetPlayersInfo(const std::vector<players::Player>& players);

} // namespace json_loader
