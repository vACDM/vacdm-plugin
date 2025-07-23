#pragma once

#include <json/json.h>

#include <string>

#include "types/Pilot.h"

namespace vacdm::interfaces {
class IBackendInterface {
   public:
    virtual ~IBackendInterface() = default;

    virtual bool patchPilot(const std::string& endpointUrl, const Json::Value& body) = 0;
    virtual bool postInitialPilotData(const types::Pilot& data) = 0;
    virtual bool sendTargetDpiNow(const types::Pilot& data) = 0;
    virtual bool sendTargetDpiTarget(const types::Pilot& data) = 0;
    virtual bool sendTargetDpiSequenced(const std::string& callsign,
                                        const std::chrono::utc_clock::time_point& asat) = 0;
    virtual bool sendAtcDpi(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt) = 0;
    virtual bool sendCustomDpiTaxioutTime(const std::string& callsign,
                                          const std::chrono::utc_clock::time_point& exot) = 0;
    virtual bool sendCustomDpiRequest(const std::string& callsign, const std::chrono::utc_clock::time_point& timePoint,
                                      const bool isAsrtUpdate) = 0;
    virtual bool sendPilotDisconnect(const std::string& callsign) = 0;
};

}  // namespace vacdm::interfaces