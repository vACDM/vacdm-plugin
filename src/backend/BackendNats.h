#pragma once

#include <nats/nats.h>

#include <memory>
#include <string>

namespace vacdm::backend {
class BackendNats {
   public:
    BackendNats(const std::string& serverUrl);
    virtual ~BackendNats();

    void send(const std::string& subject, const std::string& message);

   private:
    std::string m_serverUrl;
    natsConnection* m_connection = nullptr;
};
}  // namespace vacdm::backend
