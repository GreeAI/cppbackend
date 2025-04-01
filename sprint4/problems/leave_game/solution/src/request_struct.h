#pragma once

#include <optional>
#include <string>
#include <vector>

namespace strct {
struct Args {
  std::string destination;
  std::optional<int> tick;
  std::string root;
  std::string config;
  bool random_spawn = false;
  std::optional<std::string> state_file;
  std::optional<int> save_state_tick;
};
}; // namespace strct
