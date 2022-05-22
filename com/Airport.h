#pragma once

#include <array>
#include <string>
#include <thread>
#include <map>
#include <mutex>

#include <json/json.h>

#include <types/Flight.h>

namespace vacdm {
namespace com {

class Airport {
private:
    enum class SendType {
        Post,
        Patch,
        None
    };

    std::string m_airport;
    std::thread m_worker;
    std::mutex m_lock;
    std::map<std::string, std::array<types::Flight_t, 3>> m_flights;
    volatile bool m_stop;

    static SendType deltaEuroscopeToBackend(const std::array<types::Flight_t, 3>& data, Json::Value& root);
    static void consolidateData(std::array<types::Flight_t, 3>& data);
    void run();

public:
    Airport();
    Airport(const std::string& airport);
    ~Airport();

    const std::string& airport() const;
    void updateFromEuroscope(types::Flight_t& flight);
    void flightDisconnected(const std::string& callsign);
    void updateExot(const std::string& callsign, const std::chrono::utc_clock::time_point& exot);
    void updateTobt(const std::string& callsign, const std::chrono::utc_clock::time_point& tobt);
    bool flightExists(const std::string& callsign);
    const types::Flight_t& flight(const std::string& callsign);
};

}
}
