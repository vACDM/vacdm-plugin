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

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace vacdm {

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
 
COLORREF vACDM::colorizeEobtAndTobt(const types::Flight_t& flight) const {
    const auto now = std::chrono::utc_clock::now();
    const auto timeSinceTobt = std::chrono::duration_cast<std::chrono::seconds>(now - flight.tobt).count();
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(now - flight.tsat).count();
    const auto diffTsatTobt = std::chrono::duration_cast<std::chrono::seconds>(flight.tsat - flight.tobt).count();

    if (flight.tsat == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }
    // ASAT exists
    if (flight.asat.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    // TOBT in past && TSAT expired, i.e. 5min past TSAT || TOBT >= +1h || TSAT does not exist && TOBT in past
    // -> TOBT in past && (TSAT expired || TSAT does not exist) || TOBT >= now + 1h
    if (timeSinceTobt > 0 && (timeSinceTsat >= 5 * 60 || flight.tsat == types::defaultTime) || flight.tobt >= now + std::chrono::hours(1)) //last statement could cause problems
    {
        return this->m_pluginConfig.orange;
    }
    // Diff TOBT TSAT >= 5min && unconfirmed
    if (diffTsatTobt >= 5 * 60 && (flight.tobt_state == "GUESS" || flight.tobt_state == "FLIGHTPLAN"))
    {
        return this->m_pluginConfig.lightyellow;
    }
    // Diff TOBT TSAT >= 5min && confirmed
    if (diffTsatTobt >= 5 * 60 && flight.tobt_state == "CONFIRMED")
    {
        return this->m_pluginConfig.yellow;
    }
    // Diff TOBT TSAT < 5min
    if (diffTsatTobt < 5 * 60 && flight.tobt_state == "CONFIRMED")
    {
        return this->m_pluginConfig.green;
    }
    // tobt is not confirmed
    if (flight.tobt_state != "CONFIRMED")
    {
        return this->m_pluginConfig.lightgreen;
    }
    return this->m_pluginConfig.debug;
}

COLORREF vACDM::colorizeTsat(const types::Flight_t& flight) const {
    if (flight.asat != types::defaultTime || flight.tsat == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.tsat).count();
    if (timeSinceTsat <= 5 * 60 && timeSinceTsat >= -5 * 60)
    {
        /* CTOT not used atm
        if (flight.ctot != types::defaultTime)
        {
            // CTOT exists
            return blue;
        } */
        return this->m_pluginConfig.green;
    }
    // TSAT earlier than 5+ min
    if (timeSinceTsat < -5 * 60)
    {
        /*
        if (flight.ctot != types::defaultTime)
        {
            // CTOT exists
            return lightblue;
        } */
        return this->m_pluginConfig.lightgreen;
    }
    // TSAT passed by 5+ min
    if (timeSinceTsat > 5 * 60)
    {
        /*
        if (flight.ctot != types::defaultTime)
        {
            // CTOT exists
            return red;
        } */
        return this->m_pluginConfig.orange;
    }
    return this->m_pluginConfig.debug;
}

COLORREF vACDM::colorizeTtot(const types::Flight_t& flight) const {
    if (flight.ttot == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }

    auto now = std::chrono::utc_clock::now();

    // Round up to the next 10, 20, 30, 40, 50, or 00 minute interval
    auto rounded = std::chrono::time_point_cast<std::chrono::minutes>(flight.ttot);
    auto minute = rounded.time_since_epoch().count() % 60;
    if (minute > 0 && minute < 10) {
        rounded += std::chrono::minutes(10 - minute);
    }
    else if (minute > 10 && minute < 20) {
        rounded += std::chrono::minutes(20 - minute);
    }
    else if (minute > 20 && minute < 30) {
        rounded += std::chrono::minutes(30 - minute);
    }
    else if (minute > 30 && minute < 40) {
        rounded += std::chrono::minutes(40 - minute);
    }
    else if (minute > 40 && minute < 50) {
        rounded += std::chrono::minutes(50 - minute);
    }
    else if (minute > 50) {
        rounded += std::chrono::minutes(60 - minute);
    }
    rounded = std::chrono::time_point_cast<std::chrono::minutes>(rounded + std::chrono::seconds(30));

    // Check if the current time has passed the ttot time point
    if (flight.atot.time_since_epoch().count() > 0)
    {
        // ATOT exists
        return this->m_pluginConfig.grey;
    }
    if (now < rounded)
    {
        // time before TTOT and during TTOT block
        return this->m_pluginConfig.green;
    }
    else if (now >= rounded)
    {
        // time past TTOT / TTOT block
        return this->m_pluginConfig.orange;
    }
    return this->m_pluginConfig.debug;
}

COLORREF vACDM::colorizeAort(const types::Flight_t& flight) const {
    if (flight.aort == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }
    if (flight.aobt.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    const auto timeSinceAort = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.aort).count();

    if (timeSinceAort <= 5 * 60 && timeSinceAort >= 0)
    {
        return this->m_pluginConfig.green;
    }
    if (timeSinceAort > 5 * 60 && timeSinceAort <= 10 * 60)
    {
        return this->m_pluginConfig.yellow;
    }
    if (timeSinceAort > 10 * 60 && timeSinceAort <= 15 * 60)
    {
        return this->m_pluginConfig.orange;
    }
    if (timeSinceAort > 15 * 60)
    {
        return this->m_pluginConfig.red;
    }

    return this->m_pluginConfig.debug;
}

COLORREF vACDM::colorizeAsrt(const types::Flight_t& flight) const {

    if (flight.asat.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    const float timeSinceAsrt = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.asrt).count();
    if (timeSinceAsrt <= 5 * 60 && timeSinceAsrt >= 0)
    {
        return this->m_pluginConfig.green;
    }
    if (timeSinceAsrt > 5 * 60 && timeSinceAsrt <= 10 * 60)
    {
        return this->m_pluginConfig.yellow;
    }
    if (timeSinceAsrt > 10 * 60 && timeSinceAsrt <= 15 * 60)
    {
        return this->m_pluginConfig.orange;
    }
    if (timeSinceAsrt > 15 * 60)
    {
        return this->m_pluginConfig.red;
    }

    return this->m_pluginConfig.debug;
}

COLORREF vACDM::colorizeAobt(const types::Flight_t& flight) const {
    std::ignore = flight;
    return this->m_pluginConfig.grey;
}

COLORREF vACDM::colorizeAsat(const types::Flight_t& flight) const {
    if (flight.asat == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }

    if (flight.aobt.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }

    const auto timeSinceAsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.asat).count();
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.tsat).count();
    if (flight.taxizoneIsTaxiout == false)
    {

        if (/* Datalink clearance == true &&*/ timeSinceTsat >= -5 * 60 && timeSinceTsat <= 5 * 60)
        {
            return this->m_pluginConfig.green;
        }
        if (timeSinceAsat < 5 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    if (flight.taxizoneIsTaxiout == true)
    {
        if (timeSinceTsat >= -5 * 60 && timeSinceTsat <= 10 * 60 /* && Datalink clearance == true*/)
        {
            return this->m_pluginConfig.green;
        }
        if (timeSinceAsat < 10 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    return this->m_pluginConfig.orange;
}

COLORREF vACDM::colorizeAsatTimer(const types::Flight_t& flight) const {
    // aort set
    if (flight.aort.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    const auto timeSinceAobt = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.aobt).count();
    if (timeSinceAobt >= 0)
    {
        // hide Timer
    }
    const auto timeSinceAsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.asat).count();
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.tsat).count();
    // Pushback required
    if (flight.taxizoneIsTaxiout != false)
    {
        /*
        if (hasdatalinkclearance == true && timesincetsat >=5*60 && timesincetsat <=5*60)
        {
            return this->m_pluginConfig.green
        } */
        if (timeSinceAsat < 5 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    if (flight.taxizoneIsTaxiout == true)
    {
        if (timeSinceTsat >= -5 * 60 && timeSinceTsat <= 10 * 60)
        {
            return this->m_pluginConfig.green;
        }
        if (timeSinceAsat <= 10 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    return this->m_pluginConfig.orange;
}

COLORREF vACDM::colorizeCtotandCtottimer(const types::Flight_t& flight) const {
    if (flight.ctot == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }

    const auto timetoctot = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.ctot).count();
    if (timetoctot >= 5 * 60)
    {
        return this->m_pluginConfig.lightgreen;
    }
    if (timetoctot <= 5 * 60 && timetoctot >= -10 * 60)
    {
        return this->m_pluginConfig.green;
    }
    if (timetoctot < -10 * 60)
    {
        return this->m_pluginConfig.orange;
    }

    return this->m_pluginConfig.grey;
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
                        *pRGB = this->colorizeEobtAndTobt(data);
                    }
                    break;
                case itemType::TOBT:
                    if (data.tobt.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.tobt);
                        *pRGB = this->colorizeEobtAndTobt(data);
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
                        *pRGB = this->colorizeTsat(data);
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
                        *pRGB = this->colorizeTtot(data);
                    }
                    break;
                case itemType::ASAT:
                    if (data.asat.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.asat);
                        *pRGB = this->colorizeAsat(data);
                    }
                    break;
                case itemType::AOBT:
                    if (data.aobt.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.aobt);
                        *pRGB = this->colorizeAobt(data);
                    }
                    break;
                case itemType::ATOT:
                    if (data.atot.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.atot);
                        *pRGB = this->colorizeAobt(data);
                    }
                    break;
                case itemType::ASRT:
                    if (data.asrt.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.asrt);
                        *pRGB = this->colorizeAsrt(data);
                    }
                    break;
                case itemType::AORT:
                    if (data.aort.time_since_epoch().count() > 0) {
                        stream << std::format("{0:%H%M}", data.aort);
                        *pRGB = this->colorizeAort(data);
                    }
                    break;
                case itemType::EventBooking:
                    if (data.AircraftHasEventSlot == true) {
                        stream << "B";
                        *pRGB = this->m_pluginConfig.green;
                    }
                    else {
                        stream << "";
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

    // check if the controller is logged in as _OBS
    bool isObs = false;
    if (false == this->m_config.masterAsObserver && std::string_view(this->ControllerMyself().GetCallsign()).ends_with("_OBS") == true) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Server does not allow OBS as master", true, true, true, true, false);
        isObs = true;
    }

    bool inSweatbox = false;
    if (false == this->m_config.masterInSweatbox) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Server does not allow sweatbox-connections as master", true, true, true, true, false);
        inSweatbox = this->GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_DIRECT;
    }

    if (std::string::npos != message.find("MASTER")) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Executing vACDM as the MASTER", true, true, true, true, false);
        if (isObs == false && inSweatbox == false) {
            logging::Logger::instance().log("vACDM", logging::Logger::Level::Info, "Switched to MASTER");
            com::Server::instance().setMaster(true);
        }
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
                return true;
            }
            else if (elements[2] == "INFO") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Info);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: INFO");
                return true;
            }
            else if (elements[2] == "WARNING") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Warning);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: WARNING");
                return true;
            }
            else if (elements[2] == "ERROR") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Error);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: ERROR");
                return true;
            }
            else if (elements[2] == "CRITICAL") {
                logging::Logger::instance().setMinimumLevel(logging::Logger::Level::Critical);
                logging::Logger::instance().log("vACDM", logging::Logger::Level::System, "Switched level: CRITICAL");
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
    RegisterTagItemType("Event Booking", itemType::EventBooking);
}

} //namespace vacdm
