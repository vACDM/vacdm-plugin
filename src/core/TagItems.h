#pragma once

#include <wtypes.h>

#include <chrono>
#include <format>
#include <string>

#include "TagItemsColor.h"
#include "types/Pilot.h"

namespace vacdm::tagitems {
enum itemType {
    EOBT,
    TOBT,
    TSAT,
    TTOT,
    EXOT,
    ASAT,
    AOBT,
    ATOT,
    ASRT,
    AORT,
    CTOT,
    ECFMP_MEASURES,
    EVENT_BOOKING,
};

std::string formatTime(const std::chrono::utc_clock::time_point timepoint) {
    if (timepoint.time_since_epoch().count() > 0)
        return std::format("{:%H%M}", timepoint);
    else
        return "";
}

void displayTagItem(int ItemCode, const types::Pilot &pilot, std::stringstream &displayedText, COLORREF *textColor) {
    switch (static_cast<itemType>(ItemCode)) {
        case itemType::EOBT:
            displayedText << formatTime(pilot.eobt);
            *textColor = Color::colorizeEobt(pilot);
            break;
        case itemType::TOBT:
            displayedText << formatTime(pilot.tobt);
            *textColor = Color::colorizeTobt(pilot);
            break;
        case itemType::TSAT:
            displayedText << formatTime(pilot.tsat);
            *textColor = Color::colorizeTsat(pilot);
            break;
        case itemType::TTOT:
            displayedText << formatTime(pilot.ttot);
            *textColor = Color::colorizeTtot(pilot);
            break;
        case itemType::EXOT:
            displayedText << std::format("{:%M}", pilot.exot);
            *textColor = Color::colorizeExot(pilot);
            break;
        case itemType::ASAT:
            displayedText << formatTime(pilot.asat);
            *textColor = Color::colorizeAsat(pilot);
            break;
        case itemType::AOBT:
            displayedText << formatTime(pilot.aobt);
            *textColor = Color::colorizeAobt(pilot);
            break;
        case itemType::ATOT:
            displayedText << formatTime(pilot.atot);
            *textColor = Color::colorizeAtot(pilot);
            break;
        case itemType::ASRT:
            displayedText << formatTime(pilot.asrt);
            *textColor = Color::colorizeAsrt(pilot);
            break;
        case itemType::AORT:
            displayedText << formatTime(pilot.aort);
            *textColor = Color::colorizeAort(pilot);
            break;
        case itemType::CTOT:
            displayedText << formatTime(pilot.ctot);
            *textColor = Color::colorizeCtot(pilot);
            break;
        case itemType::ECFMP_MEASURES:
            displayedText << "";
            *textColor = Color::colorizeEcfmpMeasure(pilot);
            break;
        case itemType::EVENT_BOOKING:
            displayedText << (pilot.hasBooking ? "B" : "");
            *textColor = Color::colorizeEventBooking(pilot);
            break;
        default:
            break;
    }
}
}  // namespace vacdm::tagitems