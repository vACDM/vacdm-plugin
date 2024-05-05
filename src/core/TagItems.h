#pragma once

#include <wtypes.h>

#include <chrono>
#include <format>
#include <string>

#include "TagItemsColor.h"
#include "types/Pilot.h"
#include "vACDM.h"

using namespace vacdm::tagitems;

namespace vacdm {
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

void vACDM::RegisterTagItemTypes() {
    RegisterTagItemType("EOBT", itemType::EOBT);
    RegisterTagItemType("TOBT", itemType::TOBT);
    RegisterTagItemType("TSAT", itemType::TSAT);
    RegisterTagItemType("TTOT", itemType::TTOT);
    RegisterTagItemType("EXOT", itemType::EXOT);
    RegisterTagItemType("ASAT", itemType::ASAT);
    RegisterTagItemType("AOBT", itemType::AOBT);
    RegisterTagItemType("ATOT", itemType::ATOT);
    RegisterTagItemType("ASRT", itemType::ASRT);
    RegisterTagItemType("AORT", itemType::AORT);
    RegisterTagItemType("CTOT", itemType::CTOT);
    RegisterTagItemType("Event Booking", itemType::EVENT_BOOKING);
    RegisterTagItemType("ECFMP Measures", itemType::ECFMP_MEASURES);
}

std::string formatTime(const std::chrono::utc_clock::time_point timepoint) {
    if (timepoint.time_since_epoch().count() > 0)
        return std::format("{:%H%M}", timepoint);
    else
        return "";
}

void vACDM::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget,
                         int ItemCode, int TagData, char sItemString[16], int *pColorCode, COLORREF *pRGB,
                         double *pFontSize) {
    std::ignore = RadarTarget;
    std::ignore = TagData;
    std::ignore = pRGB;
    std::ignore = pFontSize;

    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
    if (nullptr == FlightPlan.GetFlightPlanData().GetPlanType() ||
        0 == std::strlen(FlightPlan.GetFlightPlanData().GetPlanType()))
        return;
    // skip non IFR flights
    if (std::string_view("I") != FlightPlan.GetFlightPlanData().GetPlanType()) {
        return;
    }
    std::string callsign = FlightPlan.GetCallsign();

    if (false == DataManager::instance().checkPilotExists(callsign)) return;

    auto pilot = DataManager::instance().getPilot(callsign);

    std::stringstream outputText;

    switch (static_cast<itemType>(ItemCode)) {
        case itemType::EOBT:
            outputText << formatTime(pilot.eobt);
            *pRGB = Color::instance().colorizeEobt(pilot);
            break;
        case itemType::TOBT:
            outputText << formatTime(pilot.tobt);
            *pRGB = Color::instance().colorizeTobt(pilot);
            break;
        case itemType::TSAT:
            outputText << formatTime(pilot.tsat);
            *pRGB = Color::instance().colorizeTsat(pilot);
            break;
        case itemType::TTOT:
            outputText << formatTime(pilot.ttot);
            *pRGB = Color::instance().colorizeTtot(pilot);
            break;
        case itemType::EXOT:
            if (pilot.exot.time_since_epoch().count() > 0) {
                outputText << std::format("{:%M}", pilot.exot);
                *pColorCode = Color::instance().colorizeExot(pilot);
            }
            break;
        case itemType::ASAT:
            outputText << formatTime(pilot.asat);
            *pRGB = Color::instance().colorizeAsat(pilot);
            break;
        case itemType::AOBT:
            outputText << formatTime(pilot.aobt);
            *pRGB = Color::instance().colorizeAobt(pilot);
            break;
        case itemType::ATOT:
            outputText << formatTime(pilot.atot);
            *pRGB = Color::instance().colorizeAtot(pilot);
            break;
        case itemType::ASRT:
            outputText << formatTime(pilot.asrt);
            *pRGB = Color::instance().colorizeAsrt(pilot);
            break;
        case itemType::AORT:
            outputText << formatTime(pilot.aort);
            *pRGB = Color::instance().colorizeAort(pilot);
            break;
        case itemType::CTOT:
            outputText << formatTime(pilot.ctot);
            *pRGB = Color::instance().colorizeCtot(pilot);
            break;
        case itemType::ECFMP_MEASURES:
            if (false == pilot.measures.empty()) {
                const std::int64_t measureMinutes = pilot.measures[0].value / 60;
                const std::int64_t measureSeconds = pilot.measures[0].value % 60;

                outputText << std::format("{:02}:{:02}", measureMinutes, measureSeconds);
                *pRGB = Color::instance().colorizeEcfmpMeasure(pilot);
            }
            break;
        case itemType::EVENT_BOOKING:
            outputText << (pilot.hasBooking ? "B" : "");
            *pRGB = Color::instance().colorizeEventBooking(pilot);
            break;
        default:
            break;
    }

    std::strcpy(sItemString, outputText.str().c_str());
}
}  // namespace vacdm