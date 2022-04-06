#pragma once

#include <string>

namespace vacdm {
namespace types {

typedef struct {
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
    std::string eobt;
    std::string tobt;
    std::string ctot;
    std::string ttot;
    std::string tsat;
    std::string exot;

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
