#pragma once

#include <chrono>
#include <string>

namespace vacdm {
namespace types {

static constexpr std::chrono::utc_clock::time_point defaultTime = std::chrono::utc_clock::time_point(std::chrono::milliseconds(-1));

typedef struct Flight {
    std::chrono::utc_clock::time_point lastUpdate;
    std::string callsign;
    bool inactive = false;

    // position/*
    double latitude = 0.0;
    double longitude = 0.0;

    // flightplan/*
    std::string origin;
    std::string destination;
    std::string rule;

    // vacdm/*
    std::chrono::utc_clock::time_point eobt = defaultTime;
    std::chrono::utc_clock::time_point tobt = defaultTime;
    std::chrono::utc_clock::time_point ctot = defaultTime;
    std::chrono::utc_clock::time_point ttot = defaultTime;
    std::chrono::utc_clock::time_point tsat = defaultTime;
    std::chrono::utc_clock::time_point exot = defaultTime;
    std::chrono::utc_clock::time_point asat = defaultTime;
    std::chrono::utc_clock::time_point aobt = defaultTime;
    std::chrono::utc_clock::time_point atot = defaultTime;

    // clearance/*
    std::string runway;
    std::string sid;
    std::string assignedSquawk;
    std::string currentSquawk;
    std::string initialClimb;

    bool departed = false;
} Flight_t;

}
}
