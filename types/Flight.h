#pragma once

#include <chrono>
#include <string>

namespace vacdm {
namespace types {

static constexpr std::chrono::utc_clock::time_point defaultTime = std::chrono::utc_clock::time_point(std::chrono::milliseconds(-1));

typedef struct Measure {
    std::string ident;  // measure id
    int value = -1;     // measure value in seconds, i.e. 5
} Measure;

typedef struct Flight {
    std::chrono::utc_clock::time_point lastUpdate;
    std::string callsign;
    bool inactive = false;

    // position/*
    double latitude = 0.0;
    double longitude = 0.0;
    bool taxizoneIsTaxiout = false;

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
    std::chrono::utc_clock::time_point asrt = defaultTime;
    std::chrono::utc_clock::time_point aort = defaultTime;
    std::string tobt_state = "";

    // booking/*
    bool hasBooking = false;

    // clearance/*
    std::string runway;
    std::string sid;
    std::string assignedSquawk;
    std::string currentSquawk;
    std::string initialClimb;

    bool departed = false;

    // ecfmp measures/*
    std::vector<Measure> measures;
} Flight_t;

}
}
