#pragma once

#include <array>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <list>

#include <json/json.h>

#include <types/Flight.h>

#include "PerformanceLock.h"

namespace vacdm {
namespace com {

class Airport {
private:
    enum class SendType {
        Post,
        Patch,
        Delete,
        None
    };

    struct AsynchronousMessage {
        SendType type;
        std::string callsign;
        Json::Value content;
    };

    std::string m_airport;
    std::thread m_worker;
    volatile bool m_pause;
    PerformanceLock m_lock;
    std::map<std::string, std::array<types::Flight_t, 3>> m_flights;
    volatile bool m_stop;
    logging::Performance m_manualUpdatePerformance;
    logging::Performance m_workerAllFlightsPerformance;
    logging::Performance m_workerUpdateFlightsPerformance;
    PerformanceLock m_asynchronousMessageLock;
    std::list<struct AsynchronousMessage> m_asynchronousMessages;

    void processAsynchronousMessages();
    static std::string timestampToIsoString(const std::chrono::utc_clock::time_point& timepoint);
    static SendType deltaEuroscopeToBackend(const std::array<types::Flight_t, 3>& data, Json::Value& root);
    static void consolidateData(std::array<types::Flight_t, 3>& data);
    void run();

public:
    Airport();
    Airport(const std::string& airport);
    ~Airport();

    void pause();
    void resume();
    void resetData();
    const std::string& airport() const;
    void updateFromEuroscope(types::Flight_t& flight);
    void flightDisconnected(const std::string& callsign);
    void updateExot(const std::string& callsign, const std::chrono::utc_clock::time_point& exot);
    void updateTobt(const std::string& callsign, const std::chrono::utc_clock::time_point& tobt, bool manualTobt);
    void resetTobt(const std::string& callsign, const std::chrono::utc_clock::time_point& tobt, const std::string& tobtState);
    void updateAsat(const std::string& callsign, const std::chrono::utc_clock::time_point& asat);
    void updateAobt(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt);
    void updateAtot(const std::string& callsign, const std::chrono::utc_clock::time_point& atot);
    void updateAsrt(const std::string& callsign, const std::chrono::utc_clock::time_point& asrt);
    void updateAort(const std::string& callsign, const std::chrono::utc_clock::time_point& aort);
    void deleteFlight(const std::string& callsign);
    bool flightExists(const std::string& callsign);
    const types::Flight_t& flight(const std::string& callsign);
};

}
}
