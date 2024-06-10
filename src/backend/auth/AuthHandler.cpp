#include "AuthHandler.h"

#include <shellapi.h>
#include <windows.h>

#include <fstream>

#include "backend/Server.h"
#include "config/ConfigHandler.h"
#include "log/Logger.h"
#include "main.h"
#include "utils/Json.h"

using namespace vacdm;
using namespace vacdm::com;
using namespace vacdm::logging;
using namespace std::chrono_literals;

AuthHandler::AuthHandler() : m_tokenState(TokenState::NotValid), m_stop(false) {
    this->authSocket = curl_easy_init();
    curl_easy_setopt(this->authSocket, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(this->authSocket, CURLOPT_HTTPHEADER, Server::defaultHeader);

    this->m_thread = std::thread(&AuthHandler::authThread, this);
}

AuthHandler::~AuthHandler() {
    this->m_stop = true;
    if (this->m_thread.joinable()) this->m_thread.join();

    if (nullptr != this->authSocket) {
        std::lock_guard guard(this->socketLock);
        curl_easy_cleanup(this->authSocket);
        this->authSocket = nullptr;
    }
}

AuthHandler& AuthHandler::instance() {
    static AuthHandler __instance;
    return __instance;
}

const AuthHandler::TokenState AuthHandler::getTokenState() const { return this->m_tokenState; }

void AuthHandler::authThread() {
    // check for local token
    if (this->getAuthToken() == TokenState::Valid) return;

    const auto authProcessState = this->startAuthProcess();

    if (authProcessState == AuthProcessState::Success) this->waitOnAuthProcessCompletion();
}

const AuthHandler::TokenState AuthHandler::getAuthToken() {
    // check if the file exists
    if (handlers::FileHandler::FileState::FileNotFound == handlers::FileHandler::checkFileExist(this->tokenFilePath))
        return TokenState::NotFound;

    std::ifstream stream(this->tokenFilePath);
    if (false == stream.is_open()) {
        Plugin->DisplayMessage("Could not open " + this->tokenFilePath, "Auth");
        Logger::instance().log(Logger::LogSender::AuthHandler, "Could not open " + this->tokenFilePath,
                               Logger::LogLevel::Error);
        return TokenState::Inaccessible;
    }

    // get the token from the file
    std::string authToken;
    std::getline(stream, authToken);

    // verfiy token,
    const auto tokenState = this->validateAuthToken(authToken);
    if (TokenState::Valid == tokenState) {
        Server::instance().setAuthKey(authToken);
    }
    return tokenState;
}

const AuthHandler::TokenState AuthHandler::validateAuthToken(const std::string& token) {
    if (nullptr == this->authSocket) {
        Plugin->DisplayMessage("Failed to check if auth token is still valid", "Auth");
        return TokenState::Inaccessible;
    }

    std::lock_guard guard(this->socketLock);
    struct curl_slist* headers = Server::defaultHeader;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    curl_easy_setopt(this->authSocket, CURLOPT_HTTPHEADER, headers);

    const std::string url = ConfigHandler::instance().getConfig().serverUrl + "/api/auth/profile";
    curl_easy_setopt(this->authSocket, CURLOPT_URL, url.c_str());

    CURLcode result = curl_easy_perform(this->authSocket);
    if (result != CURLE_OK) {
        return TokenState::Inaccessible;
    }

    long response_code = 0;
    curl_easy_getinfo(this->authSocket, CURLINFO_RESPONSE_CODE, &response_code);

    if (200 == response_code) {
        Logger::instance().log(Logger::LogSender::AuthHandler, "Auth key is valid, response code: " + response_code,
                               Logger::LogLevel::Info);
        Plugin->DisplayMessage("Auth key is valid, response code: " + response_code, "Auth");
        return TokenState::Valid;
    } else {
        Logger::instance().log(Logger::LogSender::AuthHandler, "Auth key is invalid, response code: " + response_code,
                               Logger::LogLevel::Info);
        Plugin->DisplayMessage("Your auth token is invalid, restarting auth process", "Auth");
        return TokenState::Expired;
    }
}

const AuthHandler::AuthProcessState AuthHandler::startAuthProcess() {
    auto response = Server::instance().postMessage(Server::instance().m_baseUrl + "/api/plugin-token/start",
                                                   Json::Value(Json::objectValue));

    if (false == utils::JsonUtil::JsonHasAllKeys(response, {"userRedirectUrl", "pollingUrl", "pollingSecret"})) {
        Plugin->DisplayMessage("Invalid response from server on auth", "Auth");
        Logger::instance().log(Logger::LogSender::AuthHandler,
                               "Invalid response from server on auth: " + response.toStyledString(),
                               Logger::LogLevel::Error);
        return AuthProcessState::Failed;
    }

    this->authPollingResponse =
        AuthPollingResponse(response["userRedirectUrl"].asString(), response["pollingUrl"].asString(),
                            response["pollingSecret"].asString());

    return AuthProcessState::Success;
}

void AuthHandler::waitOnAuthProcessCompletion() {
    ShellExecute(0, 0, this->authPollingResponse.userRedirectUrl.c_str(), 0, 0, SW_SHOW);
    Json::Value body;
    body["secret"] = this->authPollingResponse.pollingSecret;

    while (true) {
        if (true == this->m_stop) return;

        std::this_thread::sleep_for(1s);

        const auto response = Server::instance().postMessage(this->authPollingResponse.pollingUrl, body);

        if (false == response.get("ready", false).asBool()) {
            continue;
        }

        const std::string authToken = response.get("token", "").asString();

        Server::instance().setAuthKey(authToken);
        handlers::FileHandler::saveFile(authToken, this->tokenFilePath);

        return;
    }
}