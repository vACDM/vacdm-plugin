#pragma once

#include <string>

namespace vacdm {
static __inline bool isNumber(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}
}  // namespace vacdm