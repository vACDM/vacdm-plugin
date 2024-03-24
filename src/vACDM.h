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

    // Euroscope events
    void OnAirportRunwayActivityChanged() override;
    void OnTimer(int Counter) override;
    void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) override;

   private:
    std::string m_settingsPath;

    void runEuroscopeUpdate();
};

}  // namespace vacdm
