#pragma once

#include <array>
#include <chrono>
#include <string>

namespace vacdm::types {
// defines the types returned by the ECFMP API
// see https://ecfmp.vatsim.net/docs/v1 for documentation

typedef struct EcfmpMeasure_t {
    std::string ident;
    std::int64_t value = -1;
    std::vector<std::string> mandatoryRoute;
} EcfmpMeasure;

typedef struct EcfmpFilter_t {
    std::string type;
    std::vector<std::string> value;
    std::vector<std::int64_t> flightlevels;
    std::vector<std::string> waypoints;
} EcfmpFilter;

typedef struct EcfmpFlowMeasure_t {
    std::int64_t number;
    std::string ident;
    std::int64_t event_id;
    std::string reason;
    std::chrono::utc_clock::time_point starttime;
    std::chrono::utc_clock::time_point endtime;
    std::chrono::utc_clock::time_point withdrawn_at;
    std::vector<int> notified_fir_regions;
    std::vector<EcfmpMeasure> measures;
    std::vector<EcfmpFilter> filters;
} EcfmpFlowMeasure;
}  // namespace vacdm::types