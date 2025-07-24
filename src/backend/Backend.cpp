#include "Backend.h"

using namespace vacdm;
using namespace vacdm::backend;

#include <json/json.h>

#include <iostream>

#include "backend/JsonBuilder.h"

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

bool BackendWebsocket::patchPilot(const std::string& endpointUrl, const Json::Value& body) {
    (void)endpointUrl;  // using websockets we don't need the endpoint url

    m_webSocket.send(body.asString());
    return false;
}

bool BackendWebsocket::postInitialPilotData(const types::Pilot& data) {
    const auto message = JsonBuilder::buildInitialPilotData(data);

    m_webSocket.send(message.asString());

    return true;
}

bool BackendWebsocket::sendTargetDpiNow(const types::Pilot& data) {
    const auto json = JsonBuilder::buildTargetDpiNow(data);

    m_webSocket.send(json.asString());

    return true;
}

bool BackendWebsocket::sendTargetDpiTarget(const types::Pilot& data) {
    const auto json = JsonBuilder::buildTargetDpiTarget(data);

    m_webSocket.send(json.asString());

    return true;
}

bool BackendWebsocket::sendTargetDpiSequenced(const types::Pilot& data) {
    const auto json = JsonBuilder::buildTargetDpiSequenced(data);

    m_webSocket.send(json.asString());

    return true;
}

bool BackendWebsocket::sendAtcDpi(const types::Pilot& data) {
    const auto json = JsonBuilder::buildAtcDpi(data);

    m_webSocket.send(json.asString());

    return true;
}

bool BackendWebsocket::sendCustomDpiTaxioutTime(const types::Pilot& data) {
    const auto json = JsonBuilder::buildCustomDpiTaxioutTime(data);

    m_webSocket.send(json.asString());

    return true;
}

bool BackendWebsocket::sendCustomDpiRequest(const types::Pilot& data, const bool isAsrtUpdate) {
    const auto json = JsonBuilder::buildCustomDpiRequest(data, isAsrtUpdate);

    m_webSocket.send(json.asString());

    return true;
}

bool BackendWebsocket::sendPilotDisconnect(const std::string& callsign) {
    const auto json = JsonBuilder::buildPilotDisconnect(callsign);

    m_webSocket.send(json.asString());

    return false;
}
