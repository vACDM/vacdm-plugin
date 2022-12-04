#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>

namespace vacdm {
    struct SystemConfig {
        bool valid = true;
        std::string serverUrl = "https://vacdm.dotfionn.de/api/v1";
        COLORREF lightgreen = RGB(127, 252, 73);
        COLORREF lightblue = RGB(53, 218, 235);
        COLORREF green = RGB(0, 181, 27);
        COLORREF blue = RGB(0, 0, 255);
        COLORREF yellow = RGB(255, 255, 0);
        COLORREF orange = RGB(255, 153, 0);
        COLORREF red = RGB(255, 0, 0);
        COLORREF grey = RGB(153, 153, 153);
        COLORREF white = RGB(255, 255, 255);
        COLORREF debug = RGB(255, 0, 255);
    };
}
