#pragma once
#include <string_view>

namespace vacdm::utils::colors {

inline constexpr std::string_view RESET = "\033[0m";

inline constexpr std::string_view BLACK = "\033[30m";
inline constexpr std::string_view RED = "\033[31m";
inline constexpr std::string_view GREEN = "\033[32m";
inline constexpr std::string_view YELLOW = "\033[33m";
inline constexpr std::string_view BLUE = "\033[34m";
inline constexpr std::string_view MAGENTA = "\033[35m";
inline constexpr std::string_view CYAN = "\033[36m";
inline constexpr std::string_view WHITE = "\033[37m";

inline constexpr std::string_view BRIGHT_BLACK = "\033[90m";
inline constexpr std::string_view BRIGHT_RED = "\033[91m";
inline constexpr std::string_view BRIGHT_GREEN = "\033[92m";
inline constexpr std::string_view BRIGHT_YELLOW = "\033[93m";
inline constexpr std::string_view BRIGHT_BLUE = "\033[94m";
inline constexpr std::string_view BRIGHT_MAGENTA = "\033[95m";
inline constexpr std::string_view BRIGHT_CYAN = "\033[96m";
inline constexpr std::string_view BRIGHT_WHITE = "\033[97m";

}  // namespace vacdm::utils::colors
