#pragma once

#include <string>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

namespace vacdm {

class vACDM : public EuroScopePlugIn::CPlugIn {
   public:
    vACDM();
    ~vACDM();

    void DisplayMessage(const std::string &message, const std::string &sender = "vACDM");
    void SetGroundState(const EuroScopePlugIn::CFlightPlan flightplan, const std::string groundstate);
    void RegisterTagItemFuntions();

    // Euroscope events
    void OnAirportRunwayActivityChanged() override;
    void OnTimer(int Counter) override;
    void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) override;
    void OnFunctionCall(int functionId, const char *itemString, POINT pt, RECT area) override;

   private:
    std::string m_settingsPath;

    void runEuroscopeUpdate();
    void checkServerConfiguration();

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
};

}  // namespace vacdm
