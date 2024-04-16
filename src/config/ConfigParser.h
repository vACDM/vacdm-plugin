#pragma once

#include <cstdint>
#include <string>

#include "config/PluginConfig.h"

namespace vacdm {
/// @brief parses the config file .txt to type PluginConfig. Also provides information about errors.
class ConfigParser {
   private:
    std::uint32_t m_errorLine;  /* Defines the line number the error has occurred */
    std::string m_errorMessage; /* The error message to print */
    bool parseColor(const std::string &block, COLORREF &color, std::uint32_t line);

   public:
    ConfigParser();

    bool errorFound() const;
    std::uint32_t errorLine() const;
    const std::string &errorMessage() const;

    bool parse(const std::string &filename, PluginConfig &config);
};
}  // namespace vacdm