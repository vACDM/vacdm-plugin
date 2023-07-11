#include <algorithm>
#include <cstring>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <string>
#include <string_view>
#include <sstream>

#include <Windows.h>
#include <shlwapi.h>

#include <GeographicLib/Geodesic.hpp>

#include <config/FileFormat.h>
#include <com/Server.h>
#include <helper/String.h>
#include <logging/Logger.h>

#include "vACDM.h"
#include "Version.h"
#include <ui/color.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace vacdm {

void vACDM::checkServerConfiguration() {
    if (false == com::Server::instance().checkWepApi()) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Incompatible server version found!", true, true, true, true, false);
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Error message:", true, true, true, true, false);
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, com::Server::instance().errorMessage().c_str(), true, true, true, true, false);
    }
    else {
        this->m_config = com::Server::instance().serverConfiguration();
        if (this->m_config.name.length() != 0) {
            this->DisplayUserMessage("vACDM", PLUGIN_NAME, ("Connected to " + this->m_config.name).c_str(), true, true, true, true, false);
            if (true == this->m_config.masterInSweatbox)
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Master in Sweatbox allowed", true, true, true, true, false);
            if (true == this->m_config.masterAsObserver)
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Master as observer allowed", true, true, true, true, false);
        }
    }
}

vACDM::vACDM() :
        CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE),
        m_config(),
        m_activeRunways(),
        m_airportLock(),
        m_airports() {
    if (0 != curl_global_init(CURL_GLOBAL_ALL))
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Unable to initialize the network stack!", true, true, true, true, false);

    /* get the dll-path */
    char path[MAX_PATH + 1] = { 0 };
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    this->m_settingsPath = std::string(path) + "\\vacdm.txt";

    this->reloadConfiguration();

    RegisterTagItemFuntions();
    RegisterTagItemTypes();

    this->checkServerConfiguration();

    this->OnAirportRunwayActivityChanged();

    logging::Logger::instance().log("vACDM", logging::Logger::Level::Info, "Initialized vACDM");
}

vACDM::~vACDM() {
    curl_global_cleanup();
}

static __inline std::string ltrim(const std::string& str) {
    size_t start = str.find_first_not_of(" \n\r\t\f\v");
    return (start == std::string::npos) ? "" : str.substr(start);
}

static __inline std::string rtrim(const std::string& str) {
    size_t end = str.find_last_not_of(" \n\r\t\f\v");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

static __inline std::string trim(const std::string& str) {
    return rtrim(ltrim(str));
}

void vACDM::reloadConfiguration() {
    SystemConfig newConfig;
    FileFormat parser;

    if (false == parser.parse(this->m_settingsPath, newConfig) || false == newConfig.valid) {
        std::string message = "vacdm.txt:" + std::to_string(parser.errorLine()) + ": " + parser.errorMessage();
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, message.c_str(), true, true, true, true, false);
    }
    else {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Reloaded the configuration", true, true, true, true, false);
        if (this->m_pluginConfig.serverUrl != newConfig.serverUrl)
            this->changeServerUrl(newConfig.serverUrl);
        this->m_pluginConfig = newConfig;
        Color::instance().changePluginConfig(newConfig);
    }
}

void vACDM::OnAirportRunwayActivityChanged() {
    std::list<std::string> activeAirports;
    m_activeRunways.clear();

    EuroScopePlugIn::CSectorElement rwy;
    for (rwy = this->SectorFileElementSelectFirst(EuroScopePlugIn::SECTOR_ELEMENT_RUNWAY); true == rwy.IsValid();
         rwy = this->SectorFileElementSelectNext(rwy, EuroScopePlugIn::SECTOR_ELEMENT_RUNWAY)) {
        auto airport = trim(rwy.GetAirportName());

        if (true == rwy.IsElementActive(true, 0)) {
            if (m_activeRunways.find(airport) == m_activeRunways.end())
                m_activeRunways.insert({ airport, {} });
            m_activeRunways[airport].push_back(rwy.GetRunwayName(0));

            if (activeAirports.end() == std::find(activeAirports.begin(), activeAirports.end(), airport))
                activeAirports.push_back(airport);

            logging::Logger::instance().log("vACDM", logging::Logger::Level::Info, "Active runway: " + airport + ", " + rwy.GetRunwayName(0));
        }
        if (true == rwy.IsElementActive(true, 1)) {
            if (m_activeRunways.find(airport) == m_activeRunways.end())
                m_activeRunways.insert({ airport, {} });
            m_activeRunways[airport].push_back(rwy.GetRunwayName(1));

            if (activeAirports.end() == std::find(activeAirports.begin(), activeAirports.end(), airport))
                activeAirports.push_back(airport);

            logging::Logger::instance().log("vACDM", logging::Logger::Level::Info, "Active runway: " + airport + ", " + rwy.GetRunwayName(1));
        }
    }

    std::lock_guard guard(this->m_airportLock);
    for (auto it = this->m_airports.begin(); this->m_airports.end() != it;) {
        if (activeAirports.end() == std::find(activeAirports.begin(), activeAirports.end(), (*it)->airport())) {
            it = this->m_airports.erase(it);
        }
        else {
            activeAirports.remove_if([it](const std::string& airport) { return (*it)->airport() == airport; });
            ++it;
        }
    }

    for (const auto& icao : std::as_const(activeAirports))
        this->m_airports.push_back(std::unique_ptr<com::Airport>(new com::Airport(icao)));
}

EuroScopePlugIn::CRadarScreen* vACDM::OnRadarScreenCreated(const char* displayName, bool needsRadarContent, bool geoReferenced,
                                                           bool canBeSaved, bool canBeCreated) {
    std::ignore = needsRadarContent;
    std::ignore = geoReferenced;
    std::ignore = canBeSaved;
    std::ignore = canBeCreated;
    std::ignore = displayName;

    this->OnAirportRunwayActivityChanged();

    return nullptr;
}

void vACDM::OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan flightplan, const int dataType) {
    /* handle only relevant changes */
    if (EuroScopePlugIn::CTR_DATA_TYPE_TEMPORARY_ALTITUDE != dataType && EuroScopePlugIn::CTR_DATA_TYPE_SQUAWK != dataType)
        return;

    this->updateFlight(flightplan.GetCorrelatedRadarTarget());
}

void vACDM::OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget RadarTarget) {
    std::string_view origin(RadarTarget.GetCorrelatedFlightPlan().GetFlightPlanData().GetOrigin());

    {
        std::lock_guard guard(this->m_airportLock);
        for (auto& airport : this->m_airports) {
            if (airport->airport() == origin) {
                if (false == airport->flightExists(RadarTarget.GetCallsign())) {
                    break;
                }

                const auto& flightData = airport->flight(RadarTarget.GetCallsign());

                // check for AOBT and ATOT
                if (flightData.asat.time_since_epoch().count() > 0) {
                    if (flightData.aobt.time_since_epoch().count() <= 0) {
                        float distanceMeters = 0.0f;

                        GeographicLib::Geodesic::WGS84().Inverse(static_cast<float>(RadarTarget.GetPosition().GetPosition().m_Latitude), static_cast<float>(RadarTarget.GetPosition().GetPosition().m_Longitude),
                                                                 static_cast<float>(flightData.latitude), static_cast<float>(flightData.longitude), distanceMeters);

                        if (distanceMeters >= 5.0f) {
                            airport->updateAobt(RadarTarget.GetCallsign(), std::chrono::utc_clock::now());
                        }
                    }
                    else if (flightData.atot.time_since_epoch().count() <= 0) {
                        if (RadarTarget.GetGS() > 50)
                            airport->updateAtot(RadarTarget.GetCallsign(), std::chrono::utc_clock::now());
                    }
                }
                break;
            }
        }
    }

    this->updateFlight(RadarTarget);
}

void vACDM::OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan) {
    if (this->GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_DIRECT && this->GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_VIA_PROXY)
        return;

    std::lock_guard guard(this->m_airportLock);
    for (auto& airport : this->m_airports) {
        if (airport->airport() == FlightPlan.GetFlightPlanData().GetOrigin()) {
            airport->flightDisconnected(FlightPlan.GetCorrelatedRadarTarget().GetCallsign());
            break;
        }
    }
}

void vACDM::OnTimer(const int Counter) {
    if (Counter % 5 == 0)
        this->GetAircraftDetails();
}

void vACDM::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData,
                         char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize) {
    std::ignore = RadarTarget;
    std::ignore = TagData;
    std::ignore = pRGB;
    std::ignore = pFontSize;

    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
    if (nullptr == FlightPlan.GetFlightPlanData().GetPlanType() || 0 == std::strlen(FlightPlan.GetFlightPlanData().GetPlanType()))
        return;
    if (std::string_view("I") != FlightPlan.GetFlightPlanData().GetPlanType()) {
        *pRGB = (190 << 16) | (190 << 8) | 190;
        std::strcpy(sItemString, "----");
        return;
    }

    std::string_view origin(FlightPlan.GetFlightPlanData().GetOrigin());
    std::lock_guard guard(this->m_airportLock);
    for (auto& airport : this->m_airports) {
        if (airport->airport() == origin) {
            if (true == airport->flightExists(RadarTarget.GetCallsign())) {
                const auto& data = airport->flight(RadarTarget.GetCallsign());
                std::stringstream stream;

                switch (static_cast<itemType>(ItemCode)) {
                case itemType::EOBT:
                    if (data.eobt.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.eobt);
                        *pRGB = Color::instance().colorizeEobtAndTobt(data);
                    }
                    break;
                case itemType::TOBT:
                    if (data.tobt.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.tobt);
                        *pRGB = Color::instance().colorizeEobtAndTobt(data);
                    }
                    break;
                case itemType::TSAT:
                    if (data.tsat.time_since_epoch().count() > 0) {
                        if (data.asat == types::defaultTime && data.asrt.time_since_epoch().count() > 0) {
                            stream << std::format("{0:%H%M}", data.tsat) << "R"; // "R" = aircraft ready symbol
                        }
                        else {
                            stream << std::format("{0:%H%M}", data.tsat);
                        }
                        *pRGB = Color::instance().colorizeTsat(data);
                    }
                    break;
                case itemType::EXOT:
                    *pColorCode = EuroScopePlugIn::TAG_COLOR_DEFAULT;
                    if (data.exot.time_since_epoch().count() > 0)
                        stream << std::format("{0:%M}", data.exot);
                    break;
                case itemType::TTOT:
                    if (data.ttot.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.ttot);
                        *pRGB = Color::instance().colorizeTtot(data);
                    }
                    break;
                case itemType::ASAT:
                    if (data.asat.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.asat);
                        *pRGB = Color::instance().colorizeAsat(data);
                    }
                    break;
                case itemType::AOBT:
                    if (data.aobt.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.aobt);
                        *pRGB = this->m_pluginConfig.grey;
                    }
                    break;
                case itemType::ATOT:
                    if (data.atot.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.atot);
                        *pRGB = this->m_pluginConfig.grey;
                    }
                    break;
                case itemType::ASRT:
                    if (data.asrt.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.asrt);
                        *pRGB = Color::instance().colorizeAsrt(data);
                    }
                    break;
                case itemType::AORT:
                    if (data.aort.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.aort);
                        *pRGB = Color::instance().colorizeAort(data);
                    }
                    break;
                case itemType::CTOT:
                    if (data.ctot.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.ctot);
                        *pRGB = Color::instance().colorizeCtotandCtottimer(data);
                    }
                case itemType::EventBooking:
                    if (data.hasBooking == true) {
                        stream << "B";
                        *pRGB = this->m_pluginConfig.green;
                    }
                    else {
                        stream << "";
                        *pRGB = this->m_pluginConfig.grey;
                    }
                    break;
                case itemType::ECFMP_MEASURES:
                    if (data.measures.empty() == false) {
                        const int measureMinutes = data.measures[0].value / 60;
                        const int measureSeconds = data.measures[0].value % 60;

                        stream << std::format("{:02}:{:02}", measureMinutes, measureSeconds);
                        *pRGB = this->m_pluginConfig.green;
                    }
                    else {
                        stream << "------";
                        *pRGB = this->m_pluginConfig.grey;
                    }
                    break;
                default:
                    break;
                }

                std::strcpy(sItemString, stream.str().c_str());
            }
            break;
        }
    }
}

void vACDM::changeServerUrl(const std::string& url) {
    // pause all airports and reset internal data
    this->m_airportLock.lock();
    for (auto& airport : this->m_airports) {
        airport->pause();
        airport->resetData();
    }
    this->m_airportLock.unlock();

    com::Server::instance().changeServerAddress(url);
    this->DisplayUserMessage("vACDM", PLUGIN_NAME, ("Changed vACDM URL: " + url).c_str(), true, true, true, true, false);
    logging::Logger::instance().log("vACDM", logging::Logger::Level::Info, "Switched to URL: " + url);

    // validate the server
    this->checkServerConfiguration();

    // reactivate all airports
    this->m_airportLock.lock();
    for (auto& airport : this->m_airports)
        airport->resume();
    this->m_airportLock.unlock();
}

bool vACDM::OnCompileCommand(const char* sCommandLine) {
    std::string message(sCommandLine);

#pragma warning(disable: 4244)
    std::transform(message.begin(), message.end(), message.begin(), ::toupper);
#pragma warning(default: 4244)

    if (0 != message.find(".VACDM"))
        return false;

    if (std::string::npos != message.find("MASTER")) {
        bool userConnected = this->GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_NO;
        bool userIsInSweatbox = this->GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_DIRECT;
        bool userIsObs = std::string_view(this->ControllerMyself().GetCallsign()).ends_with("_OBS") == true;
        bool serverAllowsObsAsMaster = this->m_config.masterAsObserver;
        bool serverAllowsSweatboxAsMaster = this->m_config.masterInSweatbox;

        std::string userIsNotEligibleMessage;

        if (!userConnected) {
            userIsNotEligibleMessage = "You are not logged in to the VATSIM network";
        }
        else if (userIsObs && !serverAllowsObsAsMaster) {
            userIsNotEligibleMessage = "You are logged in as Observer and Server does not allow Observers to be Master";
        }
        else if (userIsInSweatbox && !serverAllowsSweatboxAsMaster) {
            userIsNotEligibleMessage = "You are logged in on a Sweatbox Server and Server does not allow Sweatbox connections";
        }
        else {
            this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Executing vACDM as the MASTER", true, true, true, true, false);
            logging::Logger::instance().log("vACDM", logging::Logger::Level::Info, "Switched to MASTER");
            com::Server::instance().setMaster(true);

            return true;
        }

        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Cannot upgrade to Master", true, true, true, true, false);
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, userIsNotEligibleMessage.c_str(), true, true, true, true, false);
        return true;
    }
    else if (std::string::npos != message.find("SLAVE")) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Executing vACDM as the SLAVE", true, true, true, true, false);
        logging::Logger::instance().log("vACDM", logging::Logger::Level::Info, "Switched to SLAVE");
        com::Server::instance().setMaster(false);
        return true;
    }
    else if (std::string::npos != message.find("RELOAD")) {
        this->reloadConfiguration();
        return true;
    }
    else if (std::string::npos != message.find("URL")) {
        const auto elements = helper::String::splitString(message, " ");
        if (3 == elements.size()) {
            this->changeServerUrl(elements[2]);
        }
        else {
            this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Unable to change vACDM URL", true, true, true, true, false);
        }
        return true;
    }
    else if (std::string::npos != message.find("LOGLEVEL")) {
        const auto elements = helper::String::splitString(message, " ");
        if (3 == elements.size()) {
            if (elements[2] == "DEBUG") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Debug);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: DEBUG");
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Switched level: DEBUG", true, true, true, true, false);
                return true;
            }
            else if (elements[2] == "INFO") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Info);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: INFO");
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Switched level: INFO", true, true, true, true, false);
                return true;
            }
            else if (elements[2] == "WARNING") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Warning);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: WARNING");
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Switched level : WARNING", true, true, true, true, false);
                return true;
            }
            else if (elements[2] == "ERROR") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Error);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: ERROR");
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Switched level: ERROR", true, true, true, true, false);
                return true;
            }
            else if (elements[2] == "CRITICAL") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Critical);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: CRITICAL");
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Switched level: CRITICAL", true, true, true, true, false);
                return true;
            }
            else if (elements[2] == "OFF") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Disabled);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: DISABLED");
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Switched level: DISABLED", true, true, true, true, false);
                return true;
            }
        }
    }

    logging::Logger::instance().log("vACDM", logging::Logger::Level::Warning, "Unknown CMDLINE: " + std::string(sCommandLine));
    return false;
}

void vACDM::DisplayDebugMessage(const std::string &message) {
    DisplayUserMessage("vACDM", "DEBUG", message.c_str(), true, false, false, false, false);
}

std::chrono::utc_clock::time_point vACDM::convertToTobt(const std::string& callsign, const std::string& eobt) {
    const auto now = std::chrono::utc_clock::now();
    if (eobt.length() == 0 || eobt.length() > 4) {
        logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug, "Uninitialized EOBT of " + callsign);
        return now;
    }

    logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug, "Converting EOBT " + eobt + " of " + callsign);

    std::stringstream stream;
    stream << std::format("{0:%Y%m%d}", now);
    std::size_t requiredLeadingZeros = 4 - eobt.length();
    while (requiredLeadingZeros != 0) {
        requiredLeadingZeros -= 1;
        stream << "0";
    }
    stream << eobt;

    std::chrono::utc_clock::time_point tobt;
    std::stringstream input(stream.str());
    std::chrono::from_stream(stream, "%Y%m%d%H%M", tobt);

    return tobt;
}

void vACDM::updateFlight(const EuroScopePlugIn::CRadarTarget& rt) {
    const auto& fp = rt.GetCorrelatedFlightPlan();

    // ignore irrelevant and non-IFR flights
    if (nullptr == fp.GetFlightPlanData().GetPlanType() || 0 == std::strlen(fp.GetFlightPlanData().GetPlanType()))
        return;
    if (std::string_view("I") != fp.GetFlightPlanData().GetPlanType())
        return;
    bool foundAirport = false;
    for (auto& airport : this->m_airports) {
        if (airport->airport() == fp.GetFlightPlanData().GetOrigin()) {
            foundAirport = true;
            break;
        }
    }
    if (!foundAirport)
        return;

    types::Flight_t flight;
    flight.callsign = rt.GetCallsign();
    flight.inactive = false;
    flight.latitude = rt.GetPosition().GetPosition().m_Latitude;
    flight.longitude = rt.GetPosition().GetPosition().m_Longitude;
    flight.origin = fp.GetFlightPlanData().GetOrigin();
    flight.destination = fp.GetFlightPlanData().GetDestination();
    flight.rule = "I";
    flight.eobt = vACDM::convertToTobt(flight.callsign, fp.GetFlightPlanData().GetEstimatedDepartureTime());
    flight.tobt = flight.eobt;
    flight.runway = fp.GetFlightPlanData().GetDepartureRwy();
    flight.sid = fp.GetFlightPlanData().GetSidName();
    flight.assignedSquawk = fp.GetControllerAssignedData().GetSquawk();
    flight.currentSquawk = rt.GetPosition().GetSquawk();
    flight.initialClimb = std::to_string(fp.GetControllerAssignedData().GetClearedAltitude());

    if (rt.GetGS() > 50) {
        logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug, flight.callsign + " departed. GS: " + std::to_string(rt.GetGS()));
        flight.departed = true;
    }

    std::lock_guard guard(this->m_airportLock);
    for (auto& airport : this->m_airports) {
        if (airport->airport() == flight.origin) {
            airport->updateFromEuroscope(flight);
            return;
        }
    }

    logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug, "Inactive ADEP: " + flight.origin);
}

void vACDM::GetAircraftDetails() {
    for (auto rt = RadarTargetSelectFirst(); rt.IsValid(); rt = RadarTargetSelectNext(rt))
        this->updateFlight(rt);
}

static __inline bool isNumber(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

void vACDM::OnFunctionCall(int functionId, const char* itemString, POINT pt, RECT area) {
    std::ignore = pt;

    auto radarTarget = this->RadarTargetSelectASEL();
    std::string callsign(radarTarget.GetCallsign());

    if ("I" != std::string_view(radarTarget.GetCorrelatedFlightPlan().GetFlightPlanData().GetPlanType()))
        return;

    std::shared_ptr<com::Airport> currentAirport;
    std::string_view origin(radarTarget.GetCorrelatedFlightPlan().GetFlightPlanData().GetOrigin());
    this->m_airportLock.lock();
    for (const auto& airport : std::as_const(this->m_airports)) {
        if (airport->airport() == origin) {
            currentAirport = airport;
            break;
        }
    }
    this->m_airportLock.unlock();

    if (nullptr == currentAirport)
        return;

    auto& data = currentAirport->flight(callsign);

    switch (static_cast<itemFunction>(functionId)) {
    case EXOT_MODIFY:
        this->OpenPopupEdit(area, static_cast<int>(itemFunction::EXOT_NEW_VALUE), itemString);
        break;
    case EXOT_NEW_VALUE:
        if (true == isNumber(itemString)) {
            const auto exot = std::chrono::utc_clock::time_point(std::chrono::minutes(std::atoi(itemString)));
            if (exot != data.exot)
                currentAirport->updateExot(callsign, exot);
        }
        break;
    case TOBT_NOW:
        currentAirport->updateTobt(callsign, std::chrono::utc_clock::now(), false);
        break;
    case TOBT_MANUAL:
        this->OpenPopupEdit(area, TOBT_MANUAL_EDIT, "");
        break;
    case TOBT_MANUAL_EDIT:
    {
        std::string clock(itemString);
        if (clock.length() == 4 && isNumber(clock)) {
            const auto hours = std::atoi(clock.substr(0, 2).c_str());
            const auto minutes = std::atoi(clock.substr(2, 4).c_str());
            if (hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60)
                currentAirport->updateTobt(callsign, this->convertToTobt(callsign, clock), true);
            else
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Invalid time format. Expected: HHMM (24 hours)", true, true, true, true, false);
        } else if (clock.length() != 0) {
            this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Invalid time format. Expected: HHMM (24 hours)", true, true, true, true, false);
        }
        break;
    }
    case ASAT_NOW:
    {
        currentAirport->updateAsat(callsign, std::chrono::utc_clock::now());
        // if ASRT has not been set yet -> set ASRT
        if (data.asrt == types::defaultTime)
        {
            currentAirport->updateAsrt(callsign, std::chrono::utc_clock::now());
        }
        break;
    }
    case ASAT_NOW_AND_STARTUP:
    {
        currentAirport->updateAsat(callsign, std::chrono::utc_clock::now());

        // if ASRT has not been set yet -> set ASRT
        if (data.asrt == types::defaultTime)
        {
            currentAirport->updateAsrt(callsign, std::chrono::utc_clock::now());
        }

        std::string scratchBackup(radarTarget.GetCorrelatedFlightPlan().GetControllerAssignedData().GetScratchPadString());
        radarTarget.GetCorrelatedFlightPlan().GetControllerAssignedData().SetScratchPadString("ST-UP");
        radarTarget.GetCorrelatedFlightPlan().GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());

        break;
    }
    case STARTUP_REQUEST:
    {
        currentAirport->updateAsrt(callsign, std::chrono::utc_clock::now());
        break;
    }
    case AOBT_NOW_AND_STATE:
    {
        // set ASRT if ASRT has not been set yet
        if (data.asrt == types::defaultTime) {
            currentAirport->updateAort(callsign, std::chrono::utc_clock::now());
        }
        currentAirport->updateAobt(callsign, std::chrono::utc_clock::now());

        // set status depending on if the aircraft is positioned at a taxi-out position
        std::string status = "";
        if (data.taxizoneIsTaxiout) {
            status = "TAXI";
        }
        else {
            status = "PUSH";
        }

        std::string scratchBackup(radarTarget.GetCorrelatedFlightPlan().GetControllerAssignedData().GetScratchPadString());
        radarTarget.GetCorrelatedFlightPlan().GetControllerAssignedData().SetScratchPadString(status.c_str());
        radarTarget.GetCorrelatedFlightPlan().GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());
        break;
    }
    case TOBT_CONFIRM:
    {
        currentAirport->updateTobt(callsign, data.tobt, true);
        break;
    }
    case OFFBLOCK_REQUEST:
    {
        currentAirport->updateAort(callsign, std::chrono::utc_clock::now());
        break;
    }
    case TOBT_MENU:
    {
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
    // Reset Functions
    RegisterTagItemFunction("Reset TOBT", RESET_TOBT);
    RegisterTagItemFunction("Reset ASAT", RESET_ASAT);
    RegisterTagItemFunction("Reset confirmed TOBT", RESET_TOBT_CONFIRM);
    RegisterTagItemFunction("Reset Offblock Request", RESET_OFFBLOCK_REQUEST);
    RegisterTagItemFunction("Reset AOBT", RESET_AOBT_AND_STATE);
}

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
    RegisterTagItemType("Event Booking", itemType::EventBooking);
    RegisterTagItemType("ECFMP Measures", itemType::ECFMP_MEASURES);
}

} //namespace vacdm
