#pragma once

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <json/json.h>

#include "types/Pilot.h"

using namespace vacdm;

namespace vacdm::core {
class DataManager {
   private:
    DataManager();

    std::thread m_worker;
    bool m_pause;
    bool m_stop;

    void run();

   public:
    ~DataManager();
    DataManager(const DataManager &) = delete;
    DataManager(DataManager &&) = delete;

    DataManager &operator=(const DataManager &) = delete;
    DataManager &operator=(DataManager &&) = delete;
    static DataManager &instance();

   private:
    std::mutex m_pilotLock;
    std::map<std::string, std::array<types::Pilot, 3>> m_pilots;
    std::mutex m_airportLock;
    std::list<std::string> m_activeAirports;

    struct EuroscopeFlightplanUpdate {
        std::chrono::utc_clock::time_point timeIssued;
        EuroScopePlugIn::CFlightPlan data;
    };

    std::mutex m_euroscopeUpdatesLock;
    std::list<EuroscopeFlightplanUpdate> m_euroscopeFlightplanUpdates;

    /// @brief consolidates all flightplan updates by throwing out old updates and keeping the most current ones
    /// @param list of flightplans to consolidate
    void consolidateFlightplanUpdates(std::list<EuroscopeFlightplanUpdate> &list);
    /// @brief updates the pilots with the saved EuroScope flightplan updates
    /// @param pilots to update
    void processEuroScopeUpdates(std::map<std::string, std::array<types::Pilot, 3U>> &pilots);
    /// @brief gathers all information from EuroScope::CFlightPlan and converts it to type Pilot
    types::Pilot CFlightPlanToPilot(const EuroScopePlugIn::CFlightPlan flightplan);
    /// @brief updates the local data with the data from the backend
    /// @param pilots to update
    void consolidateWithBackend(std::map<std::string, std::array<types::Pilot, 3U>> &pilots);
    /// @brief consolidates EuroScope and backend data
    /// @param pilot
    void consolidateData(std::array<types::Pilot, 3> &pilot);

    enum class MessageType {
        None,
        Post,
        Patch,
    };
    MessageType deltaEuroscopeToBackend(const std::array<types::Pilot, 3> &data, Json::Value &message);

   public:
    void setActiveAirports(const std::list<std::string> activeAirports);
    void queueFlightplanUpdate(EuroScopePlugIn::CFlightPlan flightplan);
};
}  // namespace vacdm::core
