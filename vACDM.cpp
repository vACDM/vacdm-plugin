#include <algorithm>
#include <ctime>

#include <com/Server.h>

#include "vACDM.h"
#include "Version.h"

namespace vacdm {

vACDM::vACDM() :
        CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE),
        m_activeRunways(),
        m_airportLock(),
        m_airports() {
    if (0 != curl_global_init(CURL_GLOBAL_ALL))
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Unable to initialize the network stack!", true, true, true, true, false);

    RegisterTagItemFuntions();
    RegisterTagItemTypes();

    if (false == com::Server::instance().checkWepApi())
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Incompatible server version found!", true, true, true, true, false);

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
                switch (static_cast<itemType>(ItemCode)) {
                case itemType::EOBT:
                    std::strcpy(sItemString, data.eobt.c_str());
                    break;
                case itemType::TOBT:
                    std::strcpy(sItemString, data.tobt.c_str());
                    break;
                case itemType::TSAT:
                    std::strcpy(sItemString, data.tsat.c_str());
                    break;
                case itemType::EXOT:
                    std::strcpy(sItemString, data.exot.c_str());
                    break;
                default:
                    std::strcpy(sItemString, data.ttot.c_str());
                    break;
                }
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
    if (std::string_view(this->ControllerMyself().GetCallsign()).ends_with("_OBS") == true)
        isObs = true;

    if (std::string::npos != message.find("MASTER")) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Executing vACDM as the MASTER", true, true, true, true, false);
        if (isObs == false)
            com::Server::instance().setMaster(true);
        return true;
    }
    else if (std::string::npos != message.find("SLAVE")) {
        this->DisplayUserMessage("vACDM", PLUGIN_NAME, "Executing vACDM as the SLAVE", true, true, true, true, false);
        com::Server::instance().setMaster(false);
        return true;
    }

    return false;
}

void vACDM::DisplayDebugMessage(const std::string &message) {
    DisplayUserMessage("vACDM", "DEBUG", message.c_str(), true, false, false, false, false);
}

std::string vACDM::createTime(int minuteOffset, int& hours, int& minutes) {
    time_t rawTime;
    struct tm* ptm;

    std::time(&rawTime);
    ptm = gmtime(&rawTime);

    ptm->tm_min += minuteOffset;
    if ((ptm->tm_min % 5) != 0)
        ptm->tm_min += 5 - (ptm->tm_min % 5);

    // correct time overlaps
    if (ptm->tm_min >= 60) {
        ptm->tm_min -= 60;
        ptm->tm_hour += 1;
        if (ptm->tm_hour >= 24)
            ptm->tm_hour -= 24;
    }

    std::stringstream stream;
    stream << std::setfill('0') << std::setw(2) << ptm->tm_hour << std::setfill('0') << std::setw(2) << ptm->tm_min;

    hours = ptm->tm_hour;
    minutes = ptm->tm_min;
    return stream.str();
}

std::string vACDM::validateEobt(const std::string& eobt) {
    int currentHours, currentMinutes;
    const std::string time = vACDM::createTime(30, currentHours, currentMinutes);

    if (eobt.length() == 4) {
        const auto hours = std::atoi(eobt.substr(0, 2).c_str());
        const auto minutes = std::atoi(eobt.substr(2, 2).c_str());

        if (hours < 12 && currentHours > 12)
            return eobt;

        // invalid EOBT -> now + 30 minutes
        if (hours < currentHours || (hours == currentHours && minutes <= currentMinutes))
            return time;
        else
            return eobt;
    }
    else {
        return time;
    }
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
    flight.eobt = vACDM::validateEobt(fp.GetFlightPlanData().GetEstimatedDepartureTime());
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

    switch (static_cast<itemFunction>(functionId)) {
    case EXOT_MODIFY:
        this->OpenPopupEdit(area, static_cast<int>(itemFunction::EXOT_NEW_VALUE), itemString);
        break;
    case EXOT_NEW_VALUE:
        if (true == isNumber(itemString))
            currentAirport->updateExot(callsign, itemString);
        break;
    case TOBT_MODIFY:
        this->OpenPopupList(area, "Modify TOBT", 1);
        this->AddPopupListElement("Ready", "", static_cast<int>(itemFunction::TOBT_NOW));
        break;
    case TOBT_NOW:
    {
        int hours, minutes;
        currentAirport->updateTobt(callsign, vACDM::createTime(0, hours, minutes));
        break;
    }
    default:
        break;
    }
}

void vACDM::RegisterTagItemFuntions() {
    RegisterTagItemFunction("Modify EXOT", EXOT_MODIFY);
    RegisterTagItemFunction("Modify TOBT", TOBT_MODIFY);
}

void vACDM::RegisterTagItemTypes() {
    RegisterTagItemType("EOBT", itemType::EOBT);
    RegisterTagItemType("TOBT", itemType::TOBT);
    RegisterTagItemType("TSAT", itemType::TSAT);
    RegisterTagItemType("TTOT", itemType::TTOT);
    RegisterTagItemType("EXOT", itemType::EXOT);
}

} //namespace vacdm
