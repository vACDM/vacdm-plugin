#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include "core/DataManager.h"
#include "types/Pilot.h"
#include "utils/Date.h"
#include "utils/Number.h"

using namespace vacdm;
using namespace vacdm::core;

namespace vacdm::tagfunctions {

enum itemFunction {
    EXOT_MODIFY = 1,
    EXOT_NEW_VALUE,
    TOBT_NOW,
    TOBT_MANUAL,
    TOBT_MANUAL_EDIT,
    TOBT_MENU,
    ASAT_NOW,
    ASAT_NOW_AND_STARTUP,
    STARTUP_REQUEST,
    TOBT_CONFIRM,
    OFFBLOCK_REQUEST,
    AOBT_NOW_AND_STATE,
};

void RegisterTagItemFuntions(vACDM *plugin) {
    plugin->RegisterTagItemFunction("Modify EXOT", EXOT_MODIFY);
    plugin->RegisterTagItemFunction("TOBT now", TOBT_NOW);
    plugin->RegisterTagItemFunction("Set TOBT", TOBT_MANUAL);
    plugin->RegisterTagItemFunction("TOBT confirm", TOBT_CONFIRM);
    plugin->RegisterTagItemFunction("Tobt menu", TOBT_MENU);
    plugin->RegisterTagItemFunction("ASAT now", ASAT_NOW);
    plugin->RegisterTagItemFunction("ASAT now and startup state", ASAT_NOW_AND_STARTUP);
    plugin->RegisterTagItemFunction("Startup Request", STARTUP_REQUEST);
    plugin->RegisterTagItemFunction("Request Offblock", OFFBLOCK_REQUEST);
    plugin->RegisterTagItemFunction("Set AOBT and Groundstate", AOBT_NOW_AND_STATE);
}

void handleTagFunction(vACDM *plugin, int functionId, const char *itemString, POINT pt, RECT area) {
    std::ignore = pt;

    auto flightplan = plugin->FlightPlanSelectASEL();
    std::string callsign(flightplan.GetCallsign());

    if (false == DataManager::instance().checkPilotExists(callsign)) return;

    auto pilot = DataManager::instance().getPilot(callsign);

    switch (static_cast<itemFunction>(functionId)) {
        case EXOT_MODIFY:
            plugin->OpenPopupEdit(area, static_cast<int>(itemFunction::EXOT_NEW_VALUE), itemString);
            break;
        case EXOT_NEW_VALUE:
            if (true == isNumber(itemString)) {
                const auto exot = std::chrono::utc_clock::time_point(std::chrono::minutes(std::atoi(itemString)));
                if (exot != pilot.exot)
                    DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateEXOT, pilot.callsign,
                                                              exot);
            }
            break;
        case TOBT_NOW:
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateTOBT, pilot.callsign,
                                                      std::chrono::utc_clock::now());
            break;
        case TOBT_MANUAL:
            plugin->OpenPopupEdit(area, TOBT_MANUAL_EDIT, "");
            break;
        case TOBT_MANUAL_EDIT: {
            std::string clock(itemString);
            if (clock.length() == 4 && isNumber(clock)) {
                const auto hours = std::atoi(clock.substr(0, 2).c_str());
                const auto minutes = std::atoi(clock.substr(2, 4).c_str());
                if (hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60)
                    DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateTOBTConfirmed,
                                                              pilot.callsign,
                                                              utils::Date::convertStringToTimePoint(clock));
                else
                    plugin->DisplayMessage("Invalid time format. Expected: HHMM (24 hours)");
            } else if (clock.length() != 0) {
                plugin->DisplayMessage("Invalid time format. Expected: HHMM (24 hours)");
            }
            break;
        }
        case ASAT_NOW: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASAT, pilot.callsign,
                                                      std::chrono::utc_clock::now());
            // if ASRT has not been set yet -> set ASRT
            if (pilot.asrt == types::defaultTime) {
                DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASRT, pilot.callsign,
                                                          std::chrono::utc_clock::now());
            }
            break;
        }
        case ASAT_NOW_AND_STARTUP: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASAT, pilot.callsign,
                                                      std::chrono::utc_clock::now());

            // if ASRT has not been set yet -> set ASRT
            if (pilot.asrt == types::defaultTime) {
                DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASRT, pilot.callsign,
                                                          std::chrono::utc_clock::now());
            }

            plugin->SetGroundState(flightplan, "ST-UP");

            break;
        }
        case STARTUP_REQUEST: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASRT, pilot.callsign,
                                                      std::chrono::utc_clock::now());
            break;
        }
        case AOBT_NOW_AND_STATE: {
            // set ASRT if ASRT has not been set yet
            if (pilot.asrt == types::defaultTime) {
                DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateAORT, pilot.callsign,
                                                          std::chrono::utc_clock::now());
            }
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateAOBT, pilot.callsign,
                                                      std::chrono::utc_clock::now());

            // set status depending on if the aircraft is positioned at a taxi-out position
            if (pilot.taxizoneIsTaxiout) {
                plugin->SetGroundState(flightplan, "TAXI");
            } else {
                plugin->SetGroundState(flightplan, "PUSH");
            }
            break;
        }
        case TOBT_CONFIRM: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateTOBTConfirmed, pilot.callsign,
                                                      pilot.tobt);
            break;
        }
        case OFFBLOCK_REQUEST: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateAORT, pilot.callsign,
                                                      std::chrono::utc_clock::now());
            break;
        }
        case TOBT_MENU: {
            plugin->OpenPopupList(area, "TOBT menu", 1);
            plugin->AddPopupListElement("TOBT now", NULL, TOBT_NOW, false, 2, false, false);
            plugin->AddPopupListElement("TOBT edit", NULL, TOBT_MANUAL, false, 2, false, false);
            plugin->AddPopupListElement("TOBT confirm", NULL, TOBT_CONFIRM, false, 2, false, false);
            break;
        }
        default:
            break;
    }
}
}  // namespace vacdm::tagfunctions