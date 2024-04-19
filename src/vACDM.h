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

    // Euroscope events
    void OnAirportRunwayActivityChanged() override;
    void OnTimer(int Counter) override;
    void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) override;
    void OnFunctionCall(int functionId, const char *itemString, POINT pt, RECT area) override;
    void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode,
                      int TagData, char sItemString[16], int *pColorCode, COLORREF *pRGB, double *pFontSize) override;
    bool OnCompileCommand(const char *sCommandLine) override;

   private:
    std::string m_dllPath;
    std::string m_configFileName = "\\vacdm.txt";
    PluginConfig m_pluginConfig;
    void changeServerUrl(const std::string &url);

    void runEuroscopeUpdate();
    void checkServerConfiguration();
    void reloadConfiguration(bool initialLoading = false);

    void RegisterTagItemTypes();
    void RegisterTagItemFuntions();
};

}  // namespace vacdm
