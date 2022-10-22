#pragma once

#include <array>
#include <string>
#include <list>
#include <functional>
#include <map>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <com/Airport.h>
#include <com/Server.h>
#include <types/Flight.h>

namespace vacdm {

enum itemType
{
    EOBT = 1,
    TOBT,
    TSAT,
    TTOT,
    EXOT,
    ASAT,
    AOBT,
    ATOT,
};

enum itemFunction
{
    EXOT_MODIFY = 1,
    EXOT_NEW_VALUE,
    TOBT_NOW,
    TOBT_MANUAL,
    TOBT_MANUAL_EDIT,
    ASAT_NOW,
};

class vACDM : public EuroScopePlugIn::CPlugIn {
public:
    vACDM();
    ~vACDM();

private:
    com::Server::ServerConfiguration_t m_config;
    std::map<std::string, std::vector<std::string>> m_activeRunways;
    std::mutex m_airportLock;
    std::list<std::shared_ptr<com::Airport>> m_airports;

    void updateFlight(const EuroScopePlugIn::CRadarTarget& rt);
    static std::chrono::utc_clock::time_point convertToTobt(const std::string& callsign, const std::string& eobt);

    COLORREF colorizeEobtAndTobt(const types::Flight_t& flight) const;
    COLORREF colorizeTsatandAsrt(const types::Flight_t& flight) const;
    COLORREF colorizeTtot(const types::Flight_t& flight) const;
    COLORREF colorizeAobt(const types::Flight_t& flight) const;
    COLORREF colorizeAsat(const types::Flight_t& flight) const;
    COLORREF colorizeAsatTimerandAort(const types::Flight_t& flight) const;
    COLORREF colorizeCtotandCtottimer(const types::Flight_t& flight) const;

    static constexpr COLORREF lightgreen = RGB(127, 252, 73);
    static constexpr COLORREF lightblue = RGB(53, 218, 235);
    static constexpr COLORREF green = RGB(0, 181, 27);
    static constexpr COLORREF blue = RGB(0, 0, 255);
    static constexpr COLORREF yellow = RGB(255, 255, 0);
    static constexpr COLORREF orange = RGB(255, 153, 0);
    static constexpr COLORREF red = RGB(255, 0, 0);
    static constexpr COLORREF grey = RGB(153, 153, 153);
    static constexpr COLORREF white = RGB(255, 255, 255);
    static constexpr COLORREF debug = RGB(255, 0, 255);

    EuroScopePlugIn::CRadarScreen* OnRadarScreenCreated(const char* displayName, bool needsRadarContent, bool geoReferenced,
                                                        bool canBeSaved, bool canBeCreated) override;
    void OnAirportRunwayActivityChanged() override;
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan flightplan, const int dataType) override;
    void OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget RadarTarget) override;
    void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnTimer(const int Counter) override;
    void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData,
                      char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize) override;
    void OnFunctionCall(int functionId, const char* itemString, POINT pt, RECT area) override;
    bool OnCompileCommand(const char* sCommandLine) override;

    void DisplayDebugMessage(const std::string &message);
    void GetAircraftDetails();
    void RegisterTagItemFuntions();
    void RegisterTagItemTypes();
};

} //namespace vacdm
