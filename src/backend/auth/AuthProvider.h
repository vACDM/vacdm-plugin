#pragma once

#include <memory>
#include <string>

#include "IAuthProvider.h"
#include "api/IRestClient.h"
#include "log/ILogger.h"

namespace auth {
class AuthProvider : public interfaces::IAuthProvider {
   private:
    std::shared_ptr<vacdm::log::ILogger> m_logger;
    std::shared_ptr<interfaces::IRestClient> m_rest;

    std::string m_authToken;
    std::string m_authTokenState;

   public:
    AuthProvider(std::shared_ptr<vacdm::log::ILogger> logger, std::shared_ptr<interfaces::IRestClient> rest);
    ~AuthProvider() override = default;

    const TokenState& getAuthTokenState() override;
    const std::string& getAuthToken() override;
    const TokenState& validateAuthToken(const std::string& token) override;
};
}  // namespace auth