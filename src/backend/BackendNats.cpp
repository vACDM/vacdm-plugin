#include "BackendNats.h"

#include <iostream>

#include "JsonBuilder.h"

using namespace vacdm::backend;

BackendNats::BackendNats(const std::string& serverUrl) : m_serverUrl(serverUrl) {
    natsStatus status = natsConnection_ConnectTo(&m_connection, m_serverUrl.c_str());
    if (status != NATS_OK) {
        std::cerr << "Failed to connect to NATS: " << natsStatus_GetText(status) << std::endl;
        m_connection = nullptr;
    } else {
        std::cout << "Connected to NATS server: " << m_serverUrl << std::endl;
    }
}

BackendNats::~BackendNats() {
    if (m_connection != nullptr) {
        natsConnection_Destroy(m_connection);
        m_connection = nullptr;
    }
}

void BackendNats::send(const std::string& subject, const std::string& message) {
    if (m_connection == nullptr) {
        std::cerr << "Not connected to NATS server." << std::endl;
        return;
    }

    natsStatus status = natsConnection_PublishString(m_connection, subject.c_str(), message.c_str());
    if (status != NATS_OK) {
        std::cerr << "Failed to send message: " << natsStatus_GetText(status) << std::endl;
    } else {
        std::cout << "Message sent to subject [" << subject << "]: " << message << std::endl;
    }
}

bool BackendNats::patchPilot(const std::string& endpointUrl, const Json::Value& body) {
    (void)endpointUrl;

    Json::StreamWriterBuilder builder;

    this->send("Test", Json::writeString(builder, message));

    this->send("", body.asString());

    return false;
}

bool BackendNats::postInitialPilotData(const types::Pilot& data) {
    const auto message = JsonBuilder::buildInitialPilotData(data);

    Json::StreamWriterBuilder builder;

    this->send("Test", Json::writeString(builder, message));

    return true;
}

bool BackendNats::sendTargetDpiNow(const types::Pilot& data) {
    const auto json = JsonBuilder::buildTargetDpiNow(data);

    this->send("", json.asString());

    return true;
}

bool BackendNats::sendTargetDpiTarget(const types::Pilot& data) {
    const auto json = JsonBuilder::buildTargetDpiTarget(data);

    this->send("", json.asString());

    return true;
}

bool BackendNats::sendTargetDpiSequenced(const types::Pilot& data) {
    const auto json = JsonBuilder::buildTargetDpiSequenced(data);

    this->send("", json.asString());

    return true;
}

bool BackendNats::sendAtcDpi(const types::Pilot& data) {
    const auto json = JsonBuilder::buildAtcDpi(data);

    this->send("", json.asString());

    return true;
}

bool BackendNats::sendCustomDpiTaxioutTime(const types::Pilot& data) {
    const auto json = JsonBuilder::buildCustomDpiTaxioutTime(data);

    this->send("", json.asString());

    return true;
}

bool BackendNats::sendCustomDpiRequest(const types::Pilot& data, const bool isAsrtUpdate) {
    const auto json = JsonBuilder::buildCustomDpiRequest(data, isAsrtUpdate);

    this->send("", json.asString());

    return true;
}

bool BackendNats::sendPilotDisconnect(const std::string& callsign) {
    const auto json = JsonBuilder::buildPilotDisconnect(callsign);

    this->send("", json.asString());

    return false;
}
