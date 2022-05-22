#pragma once

#include <chrono>
#include <string>

namespace vacdm {
namespace types {

typedef struct {
    std::chrono::utc_clock::time_point lastUpdate;
    std::string callsign;
    bool inactive;

    // position/*
    double latitude;
    double longitude;

    // flightplan/*
    std::string origin;
    std::string destination;
    std::string rule;

    // vacdm/*
    std::chrono::utc_clock::time_point eobt;
    std::chrono::utc_clock::time_point tobt;
    std::chrono::utc_clock::time_point ctot;
    std::chrono::utc_clock::time_point ttot;
    std::chrono::utc_clock::time_point tsat;
    std::chrono::utc_clock::time_point exot;

    // clearance/*
    std::string runway;
    std::string sid;
    std::string assignedSquawk;
    std::string currentSquawk;
    std::string initialClimb;

    bool departed;
} Flight_t;

}
}
