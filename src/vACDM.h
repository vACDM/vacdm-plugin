#pragma once

#include <string>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include "config/ConfigParser.h"

namespace vacdm {

class vACDM : public EuroScopePlugIn::CPlugIn {
   public:
    vACDM();
    ~vACDM();

    void DisplayMessage(const std::string &message, const std::string &sender = "vACDM");
    void SetGroundState(const EuroScopePlugIn::CFlightPlan flightplan, const std::string groundstate);
    void RegisterTagItemFuntions();
    void RegisterTagItemTypes();

    // Euroscope events
    void OnAirportRunwayActivityChanged() override;
    void OnTimer(int Counter) override;
    void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) override;
    void OnFunctionCall(int functionId, const char *itemString, POINT pt, RECT area) override;
    void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode,
                      int TagData, char sItemString[16], int *pColorCode, COLORREF *pRGB, double *pFontSize) override;

   private:
    std::string m_dllPath;
    std::string m_configFileName = "\\vacdm.txt";
    PluginConfig m_pluginConfig;
    void changeServerUrl(const std::string &url);

    void runEuroscopeUpdate();
    void checkServerConfiguration();
    void reloadConfiguration(bool initialLoading = false);

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
