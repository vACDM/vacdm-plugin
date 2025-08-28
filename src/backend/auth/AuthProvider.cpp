#include "AuthProvider.h"

#include "utils/Http.h"

using namespace auth;

AuthProvider::AuthProvider(std::shared_ptr<vacdm::log::ILogger> logger, std::shared_ptr<interfaces::IRestClient> rest)
    : m_logger(logger), m_rest(rest) {}

AuthProvider::~AuthProvider() {}

const std::string& AuthProvider::getAuthToken() {
    // TODO: retrieve token, check if authtoken was saved

    // TODO: check if authtoken is valid

    this->validateAuthToken(std::string());

    // TODO: else request token

    return std::string();
}

const AuthProvider::TokenState& AuthProvider::validateAuthToken(const std::string& token) {
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + token;

    // TODO: get url from where?!
    const auto response = m_rest->get("", headers);

    if (response.statusCode == http::kOk) {
        return TokenState::Valid;
    }
}