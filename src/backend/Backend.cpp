#include "Backend.h"

using namespace vacdm;
using namespace vacdm::backend;

#include <iostream>

BackendWebsocket::BackendWebsocket(const std::string& serverUrl) : m_serverUrl(serverUrl) {
    ix::initNetSystem();
    std::cout << "init" << std::flush;

    m_webSocket.setUrl(serverUrl);

    m_webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::cout << "received message: " << msg->str << std::endl;
            std::cout << "> " << std::flush;
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            std::cout << "Connection established" << std::endl;
            std::cout << "> " << std::flush;
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            // Maybe SSL is not configured properly
            std::cout << "Connection error: " << msg->errorInfo.reason << std::endl;
            std::cout << "> " << std::flush;
        }
    });
    m_webSocket.setPingInterval(45);

    m_webSocket.start();
}

void BackendWebsocket::send() { m_webSocket.send("hello world"); }

BackendWebsocket::~BackendWebsocket() {
    m_webSocket.stop();
    ix::uninitNetSystem();
}

bool BackendWebsocket::patchPilot(const std::string& endpointUrl, const Json::Value& body) { return false; }

bool BackendWebsocket::postInitialPilotData(const types::Pilot& data) { return false; }

bool BackendWebsocket::sendTargetDpiNow(const types::Pilot& data) { return false; }

bool BackendWebsocket::sendTargetDpiTarget(const types::Pilot& data) { return false; }

bool BackendWebsocket::sendTargetDpiSequenced(const std::string& callsign,
                                              const std::chrono::utc_clock::time_point& asat) {
    return false;
}

bool BackendWebsocket::sendAtcDpi(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt) {
    return false;
}

bool BackendWebsocket::sendCustomDpiTaxioutTime(const std::string& callsign,
                                                const std::chrono::utc_clock::time_point& exot) {
    return false;
}

bool BackendWebsocket::sendCustomDpiRequest(const std::string& callsign,
                                            const std::chrono::utc_clock::time_point& timePoint,
                                            const bool isAsrtUpdate) {
    return false;
}

bool BackendWebsocket::sendPilotDisconnect(const std::string& callsign) { return false; }
