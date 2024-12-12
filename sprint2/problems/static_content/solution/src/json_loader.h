#pragma once

#include <filesystem>

#include "model.h"

namespace boost {
       namespace json {
           class array;
       }
   }

namespace json_loader {


model::Game LoadGame(const std::filesystem::path& json_path);
void LoadingRoadIntoMap(model::Map& map, const boost::json::array& roads_array);
void LoadingBuildingsIntoMap(model::Map& map, const boost::json::array& buildings_array);
void LoadingOfficesIntoMap(model::Map& map, const boost::json::array& offices_array);

const std::string MapIdName(const model::Game::Maps& maps);
const std::string MapFullInfo(const model::Game::Maps& maps);
std::string StatusCodeProcessing(int code);

}  // namespace json_loader
