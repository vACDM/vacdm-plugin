#include "Server.h"

#include <numeric>

#include "Version.h"
#include "log/Logger.h"
#include "utils/Date.h"

using namespace vacdm;
using namespace vacdm::com;
using namespace vacdm::logging;

static std::string __receivedDeleteData;
static std::string __receivedGetData;
static std::string __receivedPatchData;
static std::string __receivedPostData;

static std::size_t receiveCurlDelete(void* ptr, std::size_t size, std::size_t nmemb, void* stream) {
    (void)stream;

    std::string serverResult = static_cast<char*>(ptr);
    __receivedDeleteData += serverResult;
    return size * nmemb;
}

static std::size_t receiveCurlGet(void* ptr, std::size_t size, std::size_t nmemb, void* stream) {
    (void)stream;

    std::string serverResult = static_cast<char*>(ptr);
    __receivedGetData += serverResult;
    return size * nmemb;
}

static std::size_t receiveCurlPatch(void* ptr, std::size_t size, std::size_t nmemb, void* stream) {
    (void)stream;

    std::string serverResult = static_cast<char*>(ptr);
    __receivedPatchData += serverResult;
    return size * nmemb;
}

static std::size_t receiveCurlPost(void* ptr, std::size_t size, std::size_t nmemb, void* stream) {
    (void)stream;

    std::string serverResult = static_cast<char*>(ptr);
    __receivedPostData += serverResult;
    return size * nmemb;
}

Server::Server()
    : m_authToken(),
      m_getRequest(),
      m_postRequest(),
      m_patchRequest(),
      m_deleteRequest(),
      m_apiIsChecked(false),
      m_apiIsValid(false),
      m_baseUrl("https://vacdm-dev.vatsim-germany.org"),
      m_clientIsMaster(false),
      m_errorCode() {
    /* configure the get request */
    curl_easy_setopt(m_getRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlGet);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_TIMEOUT, 2L);

    /* configure the post request */
    curl_easy_setopt(m_postRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_postRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_postRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_postRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlPost);
    curl_easy_setopt(m_postRequest.socket, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(m_postRequest.socket, CURLOPT_VERBOSE, 1);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + this->m_authToken).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(m_postRequest.socket, CURLOPT_HTTPHEADER, headers);

    /* configure the patch request */
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlPatch);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTPHEADER, headers);

    /* configure the delete request */
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlDelete);
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_TIMEOUT, 2L);
}

Server::~Server() {
    if (nullptr != m_getRequest.socket) {
        std::lock_guard guard(m_getRequest.lock);
        curl_easy_cleanup(m_getRequest.socket);
        m_getRequest.socket = nullptr;
    }

    if (nullptr != m_postRequest.socket) {
        std::lock_guard guard(m_postRequest.lock);
        curl_easy_cleanup(m_postRequest.socket);
        m_postRequest.socket = nullptr;
    }

    if (nullptr != m_patchRequest.socket) {
        std::lock_guard guard(m_patchRequest.lock);
        curl_easy_cleanup(m_patchRequest.socket);
        m_patchRequest.socket = nullptr;
    }

    if (nullptr != m_deleteRequest.socket) {
        std::lock_guard guard(m_deleteRequest.lock);
        curl_easy_cleanup(m_deleteRequest.socket);
        m_deleteRequest.socket = nullptr;
    }
}

void Server::changeServerAddress(const std::string& url) {
    this->m_baseUrl = url;
    this->m_apiIsChecked = false;
    this->m_apiIsValid = false;
}

bool Server::checkWebApi() {
    if (this->m_apiIsChecked == true) return this->m_apiIsValid;

    std::lock_guard guard(m_getRequest.lock);
    if (m_getRequest.socket == nullptr) {
        this->m_apiIsValid = false;
        return m_apiIsValid;
    }

    __receivedGetData.clear();

    std::string url = m_baseUrl + "/api/v1/version";
    curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

    // send the GET request
    CURLcode result = curl_easy_perform(m_getRequest.socket);
    if (result != CURLE_OK) {
        this->m_apiIsValid = false;
        return m_apiIsValid;
    }

    Json::CharReaderBuilder builder{};
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    std::string errors;
    Json::Value root;
    Logger::instance().log(Logger::LogSender::Server, "Received API-version-message: " + __receivedGetData,
                           Logger::LogLevel::Info);
    if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root,
                      &errors)) {
        if (PLUGIN_VERSION_MAJOR != root.get("major", Json::Value(-1)).asInt()) {
            this->m_errorCode = "Backend-version is incompatible. Please update the plugin.";
            this->m_apiIsValid = false;
        } else {
            this->m_apiIsValid = true;
        }

    } else {
        this->m_errorCode = "Invalid backend-version response: " + __receivedGetData;
        this->m_apiIsValid = false;
    }
    m_apiIsChecked = true;
    return this->m_apiIsValid;
}

Server::ServerConfiguration Server::getServerConfig() {
    if (false == this->m_apiIsChecked || false == this->m_apiIsValid) return Server::ServerConfiguration();

    std::lock_guard guard(m_getRequest.lock);
    if (nullptr != m_getRequest.socket) {
        __receivedGetData.clear();

        std::string url = m_baseUrl + "/api/v1/config";
        curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

        /* send the command */
        CURLcode result = curl_easy_perform(m_getRequest.socket);
        if (CURLE_OK == result) {
            Json::CharReaderBuilder builder{};
            auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
            std::string errors;
            Json::Value root;

            Logger::instance().log(Logger::LogSender::Server, "Received configuration: " + __receivedGetData,
                                   Logger::LogLevel::Info);
            if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root,
                              &errors)) {
                ServerConfiguration_t config;
                config.name = root["serverName"].asString();
                config.allowMasterInSweatbox = root["allowSimSession"].asBool();
                config.allowMasterAsObserver = root["allowObsMaster"].asBool();
                return config;
            }
        }
    }

    return ServerConfiguration();
}

std::list<types::Pilot> Server::getPilots(const std::list<std::string> airports) {
    std::lock_guard guard(m_getRequest.lock);
    if (nullptr != m_getRequest.socket) {
        __receivedGetData.clear();

        std::string url = m_baseUrl + "/api/v1/pilots";
        if (airports.size() != 0) {
            url += "?adep=" +
                   std::accumulate(std::next(airports.begin()), airports.end(), airports.front(),
                                   [](const std::string& acc, const std::string& str) { return acc + "&adep=" + str; });
        }
        Logger::instance().log(Logger::LogSender::Server, url, Logger::LogLevel::Info);

        curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

        // send GET request
        CURLcode result = curl_easy_perform(m_getRequest.socket);
        if (result == CURLE_OK) {
            Json::CharReaderBuilder builder{};
            auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
            std::string errors;
            Json::Value root;

            // Logger::instance().log("Server", "Received data" + __receivedGetData, Logger::LogLevel::Full);
            if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root,
                              &errors) &&
                root.isArray()) {
                std::list<types::Pilot> pilots;

                for (const auto& pilot : std::as_const(root)) {
                    pilots.push_back(types::Pilot());

                    pilots.back().callsign = pilot["callsign"].asString();
                    pilots.back().lastUpdate = utils::Date::isoStringToTimestamp(pilot["updatedAt"].asString());
                    pilots.back().inactive = pilot["inactive"].asBool();

                    // position data
                    pilots.back().latitude = pilot["position"]["lat"].asDouble();
                    pilots.back().longitude = pilot["position"]["lon"].asDouble();
                    pilots.back().taxizoneIsTaxiout = pilot["vacdm"]["taxizoneIsTaxiout"].asBool();

                    // flightplan & clearance data
                    pilots.back().origin = pilot["flightplan"]["departure"].asString();
                    pilots.back().destination = pilot["flightplan"]["arrival"].asString();
                    pilots.back().runway = pilot["clearance"]["dep_rwy"].asString();
                    pilots.back().sid = pilot["clearance"]["sid"].asString();

                    // ACDM procedure data
                    pilots.back().eobt = utils::Date::isoStringToTimestamp(pilot["vacdm"]["eobt"].asString());
                    pilots.back().tobt = utils::Date::isoStringToTimestamp(pilot["vacdm"]["tobt"].asString());
                    pilots.back().tobt_state = pilot["vacdm"]["tobt_state"].asString();
                    pilots.back().ctot = utils::Date::isoStringToTimestamp(pilot["vacdm"]["ctot"].asString());
                    pilots.back().ttot = utils::Date::isoStringToTimestamp(pilot["vacdm"]["ttot"].asString());
                    pilots.back().tsat = utils::Date::isoStringToTimestamp(pilot["vacdm"]["tsat"].asString());
                    pilots.back().exot = utils::Date::isoStringToTimestamp(pilot["vacdm"]["exot"].asString());
                    pilots.back().asat = utils::Date::isoStringToTimestamp(pilot["vacdm"]["asat"].asString());
                    pilots.back().aobt = utils::Date::isoStringToTimestamp(pilot["vacdm"]["aobt"].asString());
                    pilots.back().atot = utils::Date::isoStringToTimestamp(pilot["vacdm"]["atot"].asString());
                    pilots.back().asrt = utils::Date::isoStringToTimestamp(pilot["vacdm"]["asrt"].asString());
                    pilots.back().aort = utils::Date::isoStringToTimestamp(pilot["vacdm"]["aort"].asString());

                    // ECFMP measures
                    Json::Value measuresArray = pilot["measures"];
                    std::vector<types::EcfmpMeasure> parsedMeasures;
                    for (const auto& measureObject : std::as_const(measuresArray)) {
                        vacdm::types::EcfmpMeasure measure;

                        measure.ident = measureObject["ident"].asString();
                        measure.value = measureObject["value"].asInt();

                        parsedMeasures.push_back(measure);
                    }
                    pilots.back().measures = parsedMeasures;

                    // event booking data
                    pilots.back().hasBooking = pilot["hasBooking"].asBool();
                }
                Logger::instance().log(Logger::LogSender::Server, "Pilots size: " + std::to_string(pilots.size()),
                                       Logger::LogLevel::Info);
                return pilots;
            } else {
                Logger::instance().log(Logger::LogSender::Server, "Error " + errors, Logger::LogLevel::Info);
            }
        }
    }

    return {};
}

bool Server::getMaster() { return this->m_clientIsMaster; }

const std::string& Server::errorMessage() const { return this->m_errorCode; }

Server& Server::instance() {
    static Server __instance;
    return __instance;
}
