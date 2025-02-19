#include "urldecode.h"

#include <stdexcept>
#include <string>

std::string UrlDecode(std::string_view str) {
  std::string decoded;
  for (int i = 0; i < str.size(); ++i) {
    if (str[i] == '%') {
      if (i + 2 > str.size()) {
        throw std::invalid_argument("Incorrect %");
      }
      char f = str[++i];
      char l = str[++i];

      if (!std::isxdigit(f) || !std::isxdigit(l)) {
        throw std::invalid_argument(
            "Invalid hex digits in sequence at position");
      }

      char number = f + l;
      int value = (std::stoi(std::string{f}, nullptr, 16) << 4) +
                  std::stoi(std::string{l}, nullptr, 16);
      decoded.push_back(static_cast<char>(value));
      continue;
    } else if (str[i] == '+') {
      decoded.push_back(' ');
      continue;
    }
    decoded.push_back(str[i]);
  }
  return decoded;
}
