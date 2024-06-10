#pragma once

#define CURL_STATICLIB 1
#include <curl/curl.h>

#include <mutex>
#include <string>
#include <thread>

#include "handlers/FileHandler.h"

using namespace vacdm;

namespace vacdm::com {
class AuthHandler {
   private:
    friend class Server;

    AuthHandler();
    ~AuthHandler();
    AuthHandler(const AuthHandler&) = delete;
    AuthHandler(AuthHandler&&) = delete;

    AuthHandler& operator=(const AuthHandler&) = delete;
    AuthHandler& operator=(AuthHandler&&) = delete;

    static AuthHandler& instance();

    /// @brief The message which will be returned to start the auth process
    typedef struct AuthPollingResponse_t {
        std::string userRedirectUrl;
        std::string pollingUrl;
        std::string pollingSecret;

        AuthPollingResponse_t(const std::string& userRedirectUrl, const std::string& pollingUrl,
                              const std::string& pollingSecret)
            : userRedirectUrl(userRedirectUrl), pollingUrl(pollingUrl), pollingSecret(pollingSecret) {}

        AuthPollingResponse_t() = default;
    } AuthPollingResponse;

    /// @brief The message format which returns on the pollingUrl
    typedef struct AuthTokenResponse_t {
        bool ready;
        std::string token;
    } AuthTokenResponse;

    enum class TokenState { Valid, NotValid, NotFound, Inaccessible, Expired };
    const TokenState getTokenState() const;

    void authThread();

   private:
    bool m_stop;
    std::thread m_thread;
    TokenState m_tokenState;

    std::mutex socketLock;
    CURL* authSocket;

    const std::string tokenFilePath = handlers::FileHandler::getAppDataPath() + "\\vACDM\\vACDM.txt";

    /// @brief checks for a local auth token
    const TokenState getAuthToken();

    enum class AuthProcessState { Failed, Success };
    /// @brief starts the auth process
    const AuthProcessState startAuthProcess();
    AuthPollingResponse authPollingResponse;

    /// @brief waits for completion of the auth process
    void waitOnAuthProcessCompletion();

    const TokenState validateAuthToken(const std::string& token);
};
}  // namespace vacdm::com