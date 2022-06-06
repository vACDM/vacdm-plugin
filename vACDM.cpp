#include <algorithm>
#include <cstring>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <string>
#include <string_view>
#include <sstream>

#include <com/Server.h>
#include <helper/String.h>

#include "vACDM.h"
#include "Version.h"

namespace vacdm {

vACDM::vACDM() :
        CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE),
        m_config(),
        m_activeRunways(),
        m_airportLock(),
        m_airports() {
    if (0 != curl_global_init(CURL_GLOBAL_ALL))
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Unable to initialize the network stack!", true, true, true, true, false);

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
        }
        if (true == rwy.IsElementActive(true, 1)) {
            if (m_activeRunways.find(airport) == m_activeRunways.end())
                m_activeRunways.insert({ airport, {} });
            m_activeRunways[airport].push_back(rwy.GetRunwayName(1));

            if (activeAirports.end() == std::find(activeAirports.begin(), activeAirports.end(), airport))
                activeAirports.push_back(airport);
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

    *pColorCode = EuroScopePlugIn::TAG_COLOR_DEFAULT;
    if (nullptr == FlightPlan.GetFlightPlanData().GetPlanType() || 0 == std::strlen(FlightPlan.GetFlightPlanData().GetPlanType()))
        return;
    if (std::string_view("I") != FlightPlan.GetFlightPlanData().GetPlanType())
        return;

    std::string_view origin(FlightPlan.GetFlightPlanData().GetOrigin());
    std::lock_guard guard(this->m_airportLock);
    for (auto& airport : this->m_airports) {
        if (airport->airport() == origin) {
            if (true == airport->flightExists(RadarTarget.GetCallsign())) {
                const auto& data = airport->flight(RadarTarget.GetCallsign());
                std::stringstream stream;

                switch (static_cast<itemType>(ItemCode)) {
                case itemType::EOBT:
                    if (data.eobt.time_since_epoch().count() > 0)
                        stream << std::format("{0:%H%M}", data.eobt);
                    break;
                case itemType::TOBT:
                    if (data.tobt.time_since_epoch().count() > 0)
                        stream << std::format("{0:%H%M}", data.tobt);
                    break;
                case itemType::TSAT:
                    if (data.tsat.time_since_epoch().count() > 0)
                        stream << std::format("{0:%H%M}", data.tsat);
                    break;
                case itemType::EXOT:
                    if (data.exot.time_since_epoch().count() > 0)
                        stream << std::format("{0:%M}", data.exot);
                    break;
                case itemType::TTOT:
                    if (data.ttot.time_since_epoch().count() > 0)
                        stream << std::format("{0:%H%M}", data.ttot);
                    break;
                case itemType::ASAT:
                    if (data.asat.time_since_epoch().count() > 0)
                        stream << std::format("{0:%H%M}", data.asat);
                    break;
                case itemType::AOBT:
                    if (data.aobt.time_since_epoch().count() > 0)
                        stream << std::format("{0:%H%M}", data.aobt);
                    break;
                case itemType::ATOT:
                    if (data.atot.time_since_epoch().count() > 0)
                        stream << std::format("{0:%H%M}", data.atot);
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
        if (isObs == false && inSweatbox == false)
            com::Server::instance().setMaster(true);
        return true;
    }
    else if (std::string::npos != message.find("SLAVE")) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Executing vACDM as the SLAVE", true, true, true, true, false);
        com::Server::instance().setMaster(false);
        return true;
    }
    else if (std::string::npos != message.find("URL")) {
        // pause all airports and reset internal data
        this->m_airportLock.lock();
        for (auto& airport : this->m_airports) {
            airport->pause();
            airport->resetData();
        }
        this->m_airportLock.unlock();

        const auto elements = helper::String::splitString(message, " ");
        if (3 == elements.size()) {
            com::Server::instance().changeServerAddress(elements[2]);
            this->DisplayUserMessage("vACDM", PLUGIN_NAME, ("Changed vACDM URL: " + elements[2]).c_str(), true, true, true, true, false);
        }
        else {
            this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Unable to change vACDM URL", true, true, true, true, false);
        }

        // reactivate all airports
        this->m_airportLock.lock();
        for (auto& airport : this->m_airports)
            airport->resume();
        this->m_airportLock.unlock();
    }

    return false;
}

void vACDM::DisplayDebugMessage(const std::string &message) {
    DisplayUserMessage("vACDM", "DEBUG", message.c_str(), true, false, false, false, false);
}

std::chrono::utc_clock::time_point vACDM::convertToTobt(const std::string& eobt) {
    const auto now = std::chrono::utc_clock::now();
    if (eobt.length() == 0) {
        return now;
    }

    std::stringstream stream;
    stream << std::format("{0:%Y%m%d}", now) << eobt;

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

    types::Flight_t flight;
    flight.callsign = rt.GetCallsign();
    flight.inactive = false;
    flight.latitude = rt.GetPosition().GetPosition().m_Latitude;
    flight.longitude = rt.GetPosition().GetPosition().m_Longitude;
    flight.origin = fp.GetFlightPlanData().GetOrigin();
    flight.destination = fp.GetFlightPlanData().GetDestination();
    flight.rule = "I";
    flight.eobt = vACDM::convertToTobt(fp.GetFlightPlanData().GetEstimatedDepartureTime());
    flight.tobt = flight.eobt;
    flight.runway = fp.GetFlightPlanData().GetDepartureRwy();
    flight.sid = fp.GetFlightPlanData().GetSidName();
    flight.assignedSquawk = fp.GetControllerAssignedData().GetSquawk();
    flight.currentSquawk = rt.GetPosition().GetSquawk();
    flight.initialClimb = std::to_string(fp.GetControllerAssignedData().GetClearedAltitude());

    if (rt.GetGS() > 80)
        flight.departed = true;

    std::lock_guard guard(this->m_airportLock);
    for (auto& airport : this->m_airports) {
        if (airport->airport() == flight.origin) {
            airport->updateFromEuroscope(flight);
            break;
        }
    }
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
        currentAirport->updateTobt(callsign, std::chrono::utc_clock::now());
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
                currentAirport->updateTobt(callsign, this->convertToTobt(clock));
            else
                this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Invalid time format. Expected: HHMM (24 hours)", true, true, true, true, false);
        } else if (clock.length() != 0) {
            this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Invalid time format. Expected: HHMM (24 hours)", true, true, true, true, false);
        }
        break;
    }
    case ASAT_NOW:
        currentAirport->updateAsat(callsign, std::chrono::utc_clock::now());
        break;
    default:
        break;
    }
}

void vACDM::RegisterTagItemFuntions() {
    RegisterTagItemFunction("Modify EXOT", EXOT_MODIFY);
    RegisterTagItemFunction("TOBT now", TOBT_NOW);
    RegisterTagItemFunction("Set TOBT", TOBT_MANUAL);
    RegisterTagItemFunction("ASAT now", AOBT_NOW);
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
}

} //namespace vacdm
