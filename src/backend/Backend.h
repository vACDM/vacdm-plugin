#pragma once
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXUserAgent.h>
#include <ixwebsocket/IXWebSocket.h>

#include <memory>

#include "IBackendInterface.h"

namespace vacdm::backend {
class BackendWebsocket : public vacdm::interfaces::IBackendInterface {
   public:
    virtual ~BackendWebsocket();

    bool patchPilot(const std::string& endpointUrl, const Json::Value& body) override;
    bool postInitialPilotData(const types::Pilot& data) override;
    bool sendTargetDpiNow(const types::Pilot& data) override;
    bool sendTargetDpiTarget(const types::Pilot& data) override;
    bool sendTargetDpiSequenced(const std::string& callsign, const std::chrono::utc_clock::time_point& asat) override;
    bool sendAtcDpi(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt) override;
    bool sendCustomDpiTaxioutTime(const std::string& callsign, const std::chrono::utc_clock::time_point& exot) override;
    bool sendCustomDpiRequest(const std::string& callsign, const std::chrono::utc_clock::time_point& timePoint,
                              const bool isAsrtUpdate) override;
    bool sendPilotDisconnect(const std::string& callsign) override;

   public:
    BackendWebsocket(const std::string& serverUrl);
    void send();

   private:
    std::string m_serverUrl;
    ix::WebSocket m_webSocket;
};
}  // namespace vacdm::backend