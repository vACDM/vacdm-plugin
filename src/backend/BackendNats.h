#pragma once

#include <nats/nats.h>

#include <memory>
#include <string>

#include "IBackendInterface.h"

namespace vacdm::backend {
class BackendNats : public vacdm::interfaces::IBackendInterface {
   public:
    BackendNats(const std::string& serverUrl);
    virtual ~BackendNats();

    void send(const std::string& subject, const std::string& message);

    bool patchPilot(const std::string& endpointUrl, const Json::Value& body) override;
    bool postInitialPilotData(const types::Pilot& data) override;
    bool sendTargetDpiNow(const types::Pilot& data) override;
    bool sendTargetDpiTarget(const types::Pilot& data) override;
    bool sendTargetDpiSequenced(const types::Pilot& data) override;
    bool sendAtcDpi(const types::Pilot& data) override;
    bool sendCustomDpiTaxioutTime(const types::Pilot& data) override;
    bool sendCustomDpiRequest(const types::Pilot& data, const bool isAsrtUpdate) override;
    bool sendPilotDisconnect(const std::string& callsign) override;

   private:
    std::string m_serverUrl;
    natsConnection* m_connection = nullptr;
};
}  // namespace vacdm::backend
