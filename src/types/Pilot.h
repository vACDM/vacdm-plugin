#pragma once

#include <chrono>
#include <string>

#include "Ecfmp.h"

namespace vacdm::types {
static constexpr std::chrono::utc_clock::time_point defaultTime =
    std::chrono::utc_clock::time_point(std::chrono::milliseconds(-1));

typedef struct Pilot_t {
    std::string callsign;
    std::chrono::utc_clock::time_point lastUpdate;

    bool inactive = false;

    // position data

    double latitude = 0.0;
    double longitude = 0.0;
    bool taxizoneIsTaxiout = false;

    // flightplan & clearance data

    std::string origin;
    std::string destination;
    std::string runway;
    std::string sid;

    // ACDM procedure data

    std::chrono::utc_clock::time_point eobt = defaultTime;
    std::chrono::utc_clock::time_point tobt = defaultTime;
    std::string tobt_state;
    std::chrono::utc_clock::time_point ctot = defaultTime;
    std::chrono::utc_clock::time_point ttot = defaultTime;
    std::chrono::utc_clock::time_point tsat = defaultTime;
    std::chrono::utc_clock::time_point exot = defaultTime;
    std::chrono::utc_clock::time_point asat = defaultTime;
    std::chrono::utc_clock::time_point aobt = defaultTime;
    std::chrono::utc_clock::time_point atot = defaultTime;
    std::chrono::utc_clock::time_point asrt = defaultTime;
    std::chrono::utc_clock::time_point aort = defaultTime;

    // ECFMP Measures

    std::vector<EcfmpMeasure> measures;

    // event booking data

    bool hasBooking = false;
} Pilot;
}  // namespace vacdm::types