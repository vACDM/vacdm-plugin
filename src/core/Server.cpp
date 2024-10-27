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
      m_baseUrl("https://app.vacdm.net"),
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

            // Logger::instance().log(Logger::LogSender::Server, "Received data" + __receivedGetData,
            //                        Logger::LogLevel::Debug);
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
                    pilots.back().exot =
                        std::chrono::utc_clock::time_point(std::chrono::minutes(pilot["vacdm"]["exot"].asInt64()));
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

void Server::sendPostMessage(const std::string& endpointUrl, const Json::Value& root) {
    if (this->m_apiIsChecked == false || this->m_apiIsValid == false || this->m_clientIsMaster == false) return;

    Json::StreamWriterBuilder builder{};
    const auto message = Json::writeString(builder, root);

    Logger::instance().log(Logger::LogSender::Server,
                           "Posting " + root["callsign"].asString() + " with message: " + message,
                           Logger::LogLevel::Debug);

    std::lock_guard guard(this->m_postRequest.lock);
    if (m_postRequest.socket != nullptr) {
        std::string url = m_baseUrl + endpointUrl;
        curl_easy_setopt(m_postRequest.socket, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_postRequest.socket, CURLOPT_POSTFIELDS, message.c_str());

        curl_easy_perform(m_postRequest.socket);

        Logger::instance().log(Logger::LogSender::Server,
                               "Posted " + root["callsign"].asString() + " response: " + __receivedPostData,
                               Logger::LogLevel::Debug);
        __receivedPostData.clear();
    }
}

void Server::sendPatchMessage(const std::string& endpointUrl, const Json::Value& root) {
    if (this->m_apiIsChecked == false || this->m_apiIsValid == false || this->m_clientIsMaster == false) return;

    Json::StreamWriterBuilder builder{};
    const auto message = Json::writeString(builder, root);

    Logger::instance().log(Logger::LogSender::Server,
                           "Patching " + root["callsign"].asString() + " with message: " + message,
                           Logger::LogLevel::Debug);

    std::lock_guard guard(this->m_patchRequest.lock);
    if (m_patchRequest.socket != nullptr) {
        std::string url = m_baseUrl + endpointUrl;
        curl_easy_setopt(m_patchRequest.socket, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_patchRequest.socket, CURLOPT_POSTFIELDS, message.c_str());

        curl_easy_perform(m_patchRequest.socket);

        Logger::instance().log(Logger::LogSender::Server,
                               "Patched " + root["callsign"].asString() + " response: " + __receivedPatchData,
                               Logger::LogLevel::Debug);
        __receivedPatchData.clear();
    }
}

void Server::sendDeleteMessage(const std::string& endpointUrl) {
    if (this->m_apiIsChecked == false || this->m_apiIsValid == false || this->m_clientIsMaster == false) return;

    Json::StreamWriterBuilder builder{};

    std::lock_guard guard(this->m_deleteRequest.lock);
    if (m_deleteRequest.socket != nullptr) {
        std::string url = m_baseUrl + endpointUrl;

        curl_easy_setopt(m_deleteRequest.socket, CURLOPT_URL, url.c_str());

        curl_easy_perform(m_deleteRequest.socket);
        __receivedDeleteData.clear();
    }
}

void Server::postPilot(types::Pilot pilot) {
    Json::Value root;

    root["callsign"] = pilot.callsign;
    root["inactive"] = false;

    root["position"] = Json::Value();
    root["position"]["lat"] = pilot.latitude;
    root["position"]["lon"] = pilot.longitude;

    root["flightplan"] = Json::Value();
    root["flightplan"]["departure"] = pilot.origin;
    root["flightplan"]["arrival"] = pilot.destination;

    root["vacdm"] = Json::Value();
    root["vacdm"]["eobt"] = utils::Date::timestampToIsoString(pilot.eobt);
    root["vacdm"]["tobt"] = utils::Date::timestampToIsoString(pilot.tobt);

    root["clearance"] = Json::Value();
    root["clearance"]["dep_rwy"] = pilot.runway;
    root["clearance"]["sid"] = pilot.sid;

    this->sendPostMessage("/api/v1/pilots", root);
}

void Server::updateExot(const std::string& callsign, const std::chrono::utc_clock::time_point& exot) {
    Json::Value root;

    root["callsign"] = callsign;
    root["vacdm"] = Json::Value();
    root["vacdm"]["exot"] = std::chrono::duration_cast<std::chrono::minutes>(exot.time_since_epoch()).count();
    root["vacdm"]["tsat"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["ttot"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["asat"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["aobt"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["atot"] = utils::Date::timestampToIsoString(types::defaultTime);

    this->sendPatchMessage("/api/v1/pilots/" + callsign, root);
}

void Server::updateTobt(const types::Pilot& pilot, const std::chrono::utc_clock::time_point& tobt, bool manualTobt) {
    Json::Value root;

    bool resetTsat = (tobt == types::defaultTime && true == manualTobt) || tobt >= pilot.tsat;
    root["callsign"] = pilot.callsign;
    root["vacdm"] = Json::Value();

    root["vacdm"] = Json::Value();
    root["vacdm"]["tobt"] = utils::Date::timestampToIsoString(tobt);
    if (true == resetTsat) root["vacdm"]["tsat"] = utils::Date::timestampToIsoString(types::defaultTime);
    if (false == manualTobt) root["vacdm"]["tobt_state"] = "CONFIRMED";

    root["vacdm"]["ttot"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["asat"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["aobt"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["atot"] = utils::Date::timestampToIsoString(types::defaultTime);

    this->sendPatchMessage("/api/v1/pilots/" + pilot.callsign, root);
}

void Server::updateAsat(const std::string& callsign, const std::chrono::utc_clock::time_point& asat) {
    Json::Value root;

    root["callsign"] = callsign;
    root["vacdm"] = Json::Value();
    root["vacdm"]["asat"] = utils::Date::timestampToIsoString(asat);

    this->sendPatchMessage("/api/v1/pilots/" + callsign, root);
}

void Server::updateAsrt(const std::string& callsign, const std::chrono::utc_clock::time_point& asrt) {
    Json::Value root;

    root["callsign"] = callsign;
    root["vacdm"] = Json::Value();
    root["vacdm"]["asrt"] = utils::Date::timestampToIsoString(asrt);

    this->sendPatchMessage("/api/v1/pilots/" + callsign, root);
}

void Server::updateAobt(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt) {
    Json::Value root;

    root["callsign"] = callsign;
    root["vacdm"] = Json::Value();
    root["vacdm"]["aobt"] = utils::Date::timestampToIsoString(aobt);

    this->sendPatchMessage("/api/v1/pilots/" + callsign, root);
}

void Server::updateAort(const std::string& callsign, const std::chrono::utc_clock::time_point& aort) {
    Json::Value root;

    root["callsign"] = callsign;
    root["vacdm"] = Json::Value();
    root["vacdm"]["aort"] = utils::Date::timestampToIsoString(aort);

    this->sendPatchMessage("/api/v1/pilots/" + callsign, root);
}

void Server::resetTobt(const std::string& callsign, const std::chrono::utc_clock::time_point& tobt,
                       const std::string& tobtState) {
    Json::Value root;

    root["callsign"] = callsign;
    root["vacdm"] = Json::Value();
    root["vacdm"]["tobt"] = utils::Date::timestampToIsoString(tobt);
    root["vacdm"]["tobt_state"] = tobtState;
    root["vacdm"]["tsat"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["ttot"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["asat"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["asrt"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["aobt"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["atot"] = utils::Date::timestampToIsoString(types::defaultTime);
    root["vacdm"]["aort"] = utils::Date::timestampToIsoString(types::defaultTime);

    sendPatchMessage("/api/v1/pilots/" + callsign, root);
}

void Server::deletePilot(const std::string& callsign) { sendDeleteMessage("/api/v1/pilots/" + callsign); }

void Server::setMaster(bool master) { this->m_clientIsMaster = master; }

bool Server::getMaster() { return this->m_clientIsMaster; }

const std::string& Server::errorMessage() const { return this->m_errorCode; }

Server& Server::instance() {
    static Server __instance;
    return __instance;
}
