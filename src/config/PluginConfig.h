#pragma once

#include <Windows.h>

#include <sstream>
#include <string>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

namespace vacdm {
struct PluginConfig {
    bool valid = true;
    std::string serverUrl = "https://app.vacdm.net";
    int updateCycleSeconds = 5;
    COLORREF lightgreen = RGB(127, 252, 73);
    COLORREF lightblue = RGB(53, 218, 235);
    COLORREF green = RGB(0, 181, 27);
    COLORREF blue = RGB(0, 0, 255);
    COLORREF lightyellow = RGB(255, 255, 191);
    COLORREF yellow = RGB(255, 255, 0);
    COLORREF orange = RGB(255, 153, 0);
    COLORREF red = RGB(255, 0, 0);
    COLORREF grey = RGB(153, 153, 153);
    COLORREF white = RGB(255, 255, 255);
    COLORREF debug = RGB(255, 0, 255);
};

static std::string colorToString(COLORREF color) {
    std::ostringstream oss;
    oss << static_cast<int>(GetRValue(color)) << ',' << static_cast<int>(GetGValue(color)) << ','
        << static_cast<int>(GetBValue(color));
    return oss.str();
}

/// @brief formats an object of PluginConfig as String, allows saving to a file
static std::string configToString(const PluginConfig& config) {
    std::ostringstream oss;

    oss << "SERVER_url=" << config.serverUrl << '\n';
    oss << "UPDATE_RATE_SECONDS=" << config.updateCycleSeconds << '\n';
    oss << "COLOR_lightgreen=" << colorToString(config.lightgreen) << '\n';
    oss << "COLOR_lightblue=" << colorToString(config.lightblue) << '\n';
    oss << "COLOR_green=" << colorToString(config.green) << '\n';
    oss << "COLOR_blue=" << colorToString(config.blue) << '\n';
    oss << "COLOR_lightyellow=" << colorToString(config.lightyellow) << '\n';
    oss << "COLOR_yellow=" << colorToString(config.yellow) << '\n';
    oss << "COLOR_orange=" << colorToString(config.orange) << '\n';
    oss << "COLOR_red=" << colorToString(config.red) << '\n';
    oss << "COLOR_grey=" << colorToString(config.grey) << '\n';
    oss << "COLOR_white=" << colorToString(config.white) << '\n';
    oss << "COLOR_debug=" << colorToString(config.debug) << '\n';

    return oss.str();
}
}  // namespace vacdm