#pragma once

#include <windows.h>

#include <array>
#include <string>
#include <vector>

#include "utils/String.h"

namespace utils::colors {
static bool parseColor(const std::string& block, COLORREF& color, std::uint32_t line, std::string& errorMessage) {
    std::vector<std::string> colorValues = vacdm::utils::String::splitString(block, ",");

    if (colorValues.size() != 3) {
        errorMessage = "Invalid color config at line " + std::to_string(line);
        return false;
    }

    std::array<std::uint8_t, 3> colors;
    for (std::size_t i = 0; i < 3; ++i) {
        try {
            int val = std::stoi(colorValues[i]);
            if (val < 0 || val > 255) {
                errorMessage = "Color component out of range (0-255) at line " + std::to_string(line);
                return false;
            }
            colors[i] = static_cast<std::uint8_t>(val);
        } catch (const std::exception&) {
            errorMessage = "Non-numeric color component at line " + std::to_string(line) + ": '" + colorValues[i] + "'";
            return false;
        }
    }

    color = RGB(colors[0], colors[1], colors[2]);
    return true;
}
}  // namespace utils::colors
