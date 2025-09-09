#pragma once

#include <memory>
#include <string>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include "config/ConfigParser.h"
#include "config/IConfigHandler.h"
#include "core/DataManager.h"
#include "core/Server.h"

namespace vacdm {

class vACDM : public EuroScopePlugIn::CPlugIn {
   private:
    std::shared_ptr<vacdm::com::Server> m_server;
    std::shared_ptr<vacdm::core::DataManager> m_datamanager;
    std::shared_ptr<vacdm::log::ILogger> m_logger;
    std::shared_ptr<interfaces::IConfigHandler> m_confighandler;

   public:
    vACDM(std::shared_ptr<vacdm::com::Server> server, std::shared_ptr<vacdm::core::DataManager> datamanager,
          std::shared_ptr<vacdm::log::ILogger> logger, std::shared_ptr<interfaces::IConfigHandler> confighandler);
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
