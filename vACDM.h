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
#include <types/Flight.h>

namespace vacdm {

enum itemType
{
    EOBT = 1,
    TOBT,
    TSAT,
    TTOT,
    EXOT,
};

enum itemFunction
{
    EXOT_MODIFY = 1,
    EXOT_NEW_VALUE,
    TOBT_MODIFY,
    TOBT_NOW,
};

class vACDM : public EuroScopePlugIn::CPlugIn {
public:
    vACDM();
    ~vACDM();

private:
    std::map<std::string, std::vector<std::string>> m_activeRunways;
    std::mutex m_airportLock;
    std::list<std::shared_ptr<com::Airport>> m_airports;

    void updateFlight(const EuroScopePlugIn::CRadarTarget& rt);
    static std::string createTime(int minuteOffset, int& hours, int& minutes);
    static std::string validateEobt(const std::string& eobt);

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
