#pragma once

#include <string>

namespace interfaces {
class IAuthProvider {
   public:
    virtual ~IAuthProvider() = default;

    struct AuthPollingResponse {
        std::string userRedirectUrl;
        std::string pollingUrl;
        std::string pollingSecret;

        AuthPollingResponse(const std::string& userRedirectUrl, const std::string& pollingUrl,
                            const std::string& pollingSecret)
            : userRedirectUrl(userRedirectUrl), pollingUrl(pollingUrl), pollingSecret(pollingSecret) {}
    };

    struct AuthTokenResponse {
        bool ready;
        std::string token;
    };

    enum class TokenState { Valid, NotValid, NotFound };

    virtual const TokenState& getAuthTokenState() = 0;
    virtual const std::string& getAuthToken() = 0;
    virtual const TokenState& validateAuthToken(const std::string& token) = 0;  // TODO: can this be private?
};

}  // namespace interfaces
