#include "vACDM.h"

#include <Windows.h>
#include <shlwapi.h>

#include <numeric>

#include "Version.h"
#include "core/DataManager.h"
#include "log/Logger.h"
#include "utils/String.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace vacdm;
using namespace vacdm::core;
using namespace vacdm::logging;
using namespace vacdm::utils;

namespace vacdm {
vACDM::vACDM()
    : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE) {
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    Logger::instance().log(Logger::LogSender::vACDM, "Version " + std::string(PLUGIN_VERSION) + " loaded",
                           Logger::LogLevel::System);

    // get the dll-path
    char path[MAX_PATH + 1] = {0};
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    this->m_settingsPath = std::string(path) + "\\vacdm.txt";

    // set the active airports
    this->OnAirportRunwayActivityChanged();
}

vACDM::~vACDM() {}

void vACDM::DisplayMessage(const std::string &message, const std::string &sender) {
    DisplayUserMessage("vACDM", sender.c_str(), message.c_str(), true, false, false, false, false);
}

void vACDM::runEuroscopeUpdate() {
    for (EuroScopePlugIn::CFlightPlan flightplan = FlightPlanSelectFirst(); flightplan.IsValid();
         flightplan = FlightPlanSelectNext(flightplan)) {
        DataManager::instance().queueFlightplanUpdate(flightplan);
    }
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
}  // namespace vacdm