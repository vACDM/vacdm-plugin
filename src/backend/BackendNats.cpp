#include "BackendNats.h"

#include <iostream>

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
