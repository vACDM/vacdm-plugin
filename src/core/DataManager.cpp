#include "DataManager.h"

using namespace vacdm::core;
using namespace std::chrono_literals;

DataManager::DataManager() : m_pause(false), m_stop(false) { this->m_worker = std::thread(&DataManager::run, this); }

DataManager::~DataManager() {
    this->m_stop = true;
    this->m_worker.join();
}

DataManager& DataManager::instance() {
    static DataManager __instance;
    return __instance;
}

void DataManager::run() {
    std::size_t counter = 1;
    while (true) {
        std::this_thread::sleep_for(1s);
        if (true == this->m_stop) return;
        if (true == this->m_pause) continue;

        // run every 5 seconds
        if (counter++ % 5 != 0) return;
    }
}

void DataManager::setActiveAirports(const std::list<std::string> activeAirports) {
    std::lock_guard guard(this->m_airportLock);
    this->m_activeAirports = activeAirports;
}
