#pragma once

#include <filesystem>

#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);

const std::string MapIdName(const model::Game::Maps maps);
const std::string MapFullInfo(const model::Game::Maps maps);
std::string StatusCodeProcessing(int code);

}  // namespace json_loader
