#include "vACDM.h"

#include <Windows.h>
#include <shlwapi.h>

#include <numeric>

#include "Version.h"
#include "core/DataManager.h"
#include "core/Server.h"
#include "log/Logger.h"
#include "utils/Date.h"
#include "utils/Number.h"
#include "utils/String.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace vacdm;
using namespace vacdm::com;
using namespace vacdm::core;
using namespace vacdm::logging;
using namespace vacdm::utils;

namespace vacdm {
vACDM::vACDM()
    : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE) {
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    Logger::instance().log(Logger::LogSender::vACDM, "Version " + std::string(PLUGIN_VERSION) + " loaded",
                           Logger::LogLevel::System);

    if (0 != curl_global_init(CURL_GLOBAL_ALL)) DisplayMessage("Unable to initialize the network stack!");

    // get the dll-path
    char path[MAX_PATH + 1] = {0};
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    this->m_settingsPath = std::string(path) + "\\vacdm.txt";

    RegisterTagItemFuntions();

    this->checkServerConfiguration();
}

vACDM::~vACDM() {}

void vACDM::DisplayMessage(const std::string &message, const std::string &sender) {
    DisplayUserMessage("vACDM", sender.c_str(), message.c_str(), true, false, false, false, false);
}

void vACDM::checkServerConfiguration() {
    if (Server::instance().checkWebApi() == false) {
        DisplayMessage("Connection failed.", "Server");
        DisplayMessage(Server::instance().errorMessage().c_str(), "Server");
    } else {
        std::string serverName = Server::instance().getServerConfig().name;
        DisplayMessage(("Connected to " + serverName), "Server");
        // set active airports and runways
        this->OnAirportRunwayActivityChanged();
    }
}

void vACDM::runEuroscopeUpdate() {
    for (EuroScopePlugIn::CFlightPlan flightplan = FlightPlanSelectFirst(); flightplan.IsValid();
         flightplan = FlightPlanSelectNext(flightplan)) {
        DataManager::instance().queueFlightplanUpdate(flightplan);
    }
}

void vACDM::SetGroundState(const EuroScopePlugIn::CFlightPlan flightplan, const std::string groundstate) {
    // using GRP and default Euroscope ground states
    // STATE                    ABBREVIATION    GRP STATE
    // - No state(departure)    NSTS
    // - On Freq                ONFREQ              Y
    // - De - Ice               DE-ICE              Y
    // - Start - Up             STUP
    // - Pushback               PUSH
    // - Taxi                   TAXI
    // - Line Up                LINEUP              Y
    // - Taxi In                TXIN
    // - No state(arrival)      NOSTATE             Y
    // - Parked                 PARK

    std::string scratchBackup(flightplan.GetControllerAssignedData().GetScratchPadString());
    flightplan.GetControllerAssignedData().SetScratchPadString(groundstate.c_str());
    flightplan.GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());
}

// Euroscope Events:

void vACDM::OnTimer(int Counter) {
    if (Counter % 5 == 0) this->runEuroscopeUpdate();
}

void vACDM::OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan) {
    DataManager::instance().queueFlightplanUpdate(FlightPlan);
}

void vACDM::OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) {
    // preemptive return to only handle relevant changes
    if (EuroScopePlugIn::CTR_DATA_TYPE_SPEED == DataType || EuroScopePlugIn::CTR_DATA_TYPE_MACH == DataType ||
        EuroScopePlugIn::CTR_DATA_TYPE_RATE == DataType || EuroScopePlugIn::CTR_DATA_TYPE_HEADING == DataType ||
        EuroScopePlugIn::CTR_DATA_TYPE_DIRECT_TO == DataType) {
        return;
    }
    DataManager::instance().queueFlightplanUpdate(FlightPlan);
}

void vACDM::OnAirportRunwayActivityChanged() {
    std::list<std::string> activeAirports;

    EuroScopePlugIn::CSectorElement airport;
    for (airport = this->SectorFileElementSelectFirst(EuroScopePlugIn::SECTOR_ELEMENT_AIRPORT);
         airport.IsValid() == true;
         airport = this->SectorFileElementSelectNext(airport, EuroScopePlugIn::SECTOR_ELEMENT_AIRPORT)) {
        // skip airport if it is selected as active airport for departures or arrivals
        if (false == airport.IsElementActive(true, 0) && false == airport.IsElementActive(false, 0)) continue;

        // get the airport ICAO
        auto airportICAO = utils::String::findIcao(utils::String::trim(airport.GetName()));
        // skip airport if no ICAO has been found
        if (airportICAO == "") continue;

        // check if the airport has been added already, add if it does not exist
        if (std::find(activeAirports.begin(), activeAirports.end(), airportICAO) == activeAirports.end()) {
            activeAirports.push_back(airportICAO);
        }
    }

    if (activeAirports.empty()) {
        Logger::instance().log(Logger::LogSender::vACDM,
                               "Airport/Runway Change, no active airports: ", Logger::LogLevel::Info);
    } else {
        Logger::instance().log(
            Logger::LogSender::vACDM,
            "Airport/Runway Change, active airports: " +
                std::accumulate(std::next(activeAirports.begin()), activeAirports.end(), activeAirports.front(),
                                [](const std::string &acc, const std::string &str) { return acc + " " + str; }),
            Logger::LogLevel::Info);
    }
    DataManager::instance().setActiveAirports(activeAirports);
}
void vACDM::OnFunctionCall(int functionId, const char *itemString, POINT pt, RECT area) {
    std::ignore = pt;
    auto flightplan = this->FlightPlanSelectASEL();
    std::string callsign(flightplan.GetCallsign());

    if (false == DataManager::instance().checkPilotExists(callsign)) return;

    auto pilot = DataManager::instance().getPilot(callsign);

    switch (static_cast<itemFunction>(functionId)) {
        case EXOT_MODIFY:
            this->OpenPopupEdit(area, static_cast<int>(itemFunction::EXOT_NEW_VALUE), itemString);
            break;
        case EXOT_NEW_VALUE:
            if (true == isNumber(itemString)) {
                const auto exot = std::chrono::utc_clock::time_point(std::chrono::minutes(std::atoi(itemString)));
                if (exot != pilot.exot)
                    DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateEXOT, callsign, exot);
            }
            break;
        case TOBT_NOW:
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateTOBT, callsign,
                                                      std::chrono::utc_clock::now());
            break;
        case TOBT_MANUAL:
            this->OpenPopupEdit(area, TOBT_MANUAL_EDIT, "");
            break;
        case TOBT_MANUAL_EDIT: {
            std::string clock(itemString);
            if (clock.length() == 4 && isNumber(clock)) {
                const auto hours = std::atoi(clock.substr(0, 2).c_str());
                const auto minutes = std::atoi(clock.substr(2, 4).c_str());
                if (hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60)
                    DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateTOBTConfirmed, callsign,
                                                              utils::Date::convertStringToTimePoint(clock));
                else
                    DisplayMessage("Invalid time format. Expected: HHMM (24 hours)");
            } else if (clock.length() != 0) {
                DisplayMessage("Invalid time format. Expected: HHMM (24 hours)");
            }
            break;
        }
        case ASAT_NOW: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASAT, callsign,
                                                      std::chrono::utc_clock::now());
            // if ASRT has not been set yet -> set ASRT
            if (pilot.asrt == types::defaultTime) {
                DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASRT, callsign,
                                                          std::chrono::utc_clock::now());
            }
            break;
        }
        case ASAT_NOW_AND_STARTUP: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASAT, callsign,
                                                      std::chrono::utc_clock::now());

            // if ASRT has not been set yet -> set ASRT
            if (pilot.asrt == types::defaultTime) {
                DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASRT, callsign,
                                                          std::chrono::utc_clock::now());
            }

            SetGroundState(flightplan, "ST-UP");

            break;
        }
        case STARTUP_REQUEST: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateASRT, callsign,
                                                      std::chrono::utc_clock::now());
            break;
        }
        case AOBT_NOW_AND_STATE: {
            // set ASRT if ASRT has not been set yet
            if (pilot.asrt == types::defaultTime) {
                DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateAORT, callsign,
                                                          std::chrono::utc_clock::now());
            }
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateAOBT, callsign,
                                                      std::chrono::utc_clock::now());

            // set status depending on if the aircraft is positioned at a taxi-out position
            if (pilot.taxizoneIsTaxiout) {
                SetGroundState(flightplan, "TAXI");
            } else {
                SetGroundState(flightplan, "PUSH");
            }
            break;
        }
        case TOBT_CONFIRM: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateTOBTConfirmed, callsign,
                                                      pilot.tobt);
            break;
        }
        case OFFBLOCK_REQUEST: {
            DataManager::instance().handleTagFunction(DataManager::MessageType::UpdateAORT, callsign,
                                                      std::chrono::utc_clock::now());
            break;
        }
        case TOBT_MENU: {
            this->OpenPopupList(area, "TOBT menu", 1);
            AddPopupListElement("TOBT now", NULL, TOBT_NOW, false, 2, false, false);
            AddPopupListElement("TOBT edit", NULL, TOBT_MANUAL, false, 2, false, false);
            AddPopupListElement("TOBT confirm", NULL, TOBT_CONFIRM, false, 2, false, false);
            break;
        }
        default:
            break;
    }
}

void vACDM::RegisterTagItemFuntions() {
    RegisterTagItemFunction("Modify EXOT", EXOT_MODIFY);
    RegisterTagItemFunction("TOBT now", TOBT_NOW);
    RegisterTagItemFunction("Set TOBT", TOBT_MANUAL);
    RegisterTagItemFunction("TOBT confirm", TOBT_CONFIRM);
    RegisterTagItemFunction("Tobt menu", TOBT_MENU);
    RegisterTagItemFunction("ASAT now", ASAT_NOW);
    RegisterTagItemFunction("ASAT now and startup state", ASAT_NOW_AND_STARTUP);
    RegisterTagItemFunction("Startup Request", STARTUP_REQUEST);
    RegisterTagItemFunction("Request Offblock", OFFBLOCK_REQUEST);
    RegisterTagItemFunction("Set AOBT and Groundstate", AOBT_NOW_AND_STATE);
}

}  // namespace vacdm