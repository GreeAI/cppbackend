#pragma once

#include <optional>
#include <string>
#include <vector>

namespace strct {
struct Args {
  std::vector<std::string> source;
  std::string destination;
  std::optional<double> tick;
  std::string root;
  std::string config;
  bool random_spawn = false;
};
}; // namespace strct
