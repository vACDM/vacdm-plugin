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
    virtual bool sendTargetDpiSequenced(const types::Pilot& data) = 0;
    virtual bool sendAtcDpi(const types::Pilot& data) = 0;
    virtual bool sendCustomDpiTaxioutTime(const types::Pilot& data) = 0;
    virtual bool sendCustomDpiRequest(const types::Pilot& data, const bool isAsrtUpdate) = 0;
    virtual bool sendPilotDisconnect(const std::string& callsign) = 0;
};

}  // namespace vacdm::interfaces