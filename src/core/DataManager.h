#pragma once

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>

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

   public:
    void setActiveAirports(const std::list<std::string> activeAirports);
};
}  // namespace vacdm::core
