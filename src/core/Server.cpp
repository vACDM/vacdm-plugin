#include "Server.h"

#include <main.h>

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
      m_pause(false),
      m_apiIsChecked(false),
      m_apiIsValid(false),
      m_baseUrl("https://app.vacdm.net"),
      m_clientIsMaster(false) {
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

void Server::pause() { this->m_pause = true; }

void Server::resume() { this->m_pause = false; }

const Json::Value Server::getMessage(const std::string& url) {
    std::lock_guard guard(this->m_getRequest.lock);
    if (nullptr == this->m_getRequest.socket) return Json::Value();

    __receivedGetData.clear();
    curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

    CURLcode result = curl_easy_perform(m_getRequest.socket);
    if (CURLE_OK != result) return Json::Value();

    Json::CharReaderBuilder builder{};
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    std::string errors;
    Json::Value response;

    if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &response,
                      &errors)) {
        return response;
    }
    Logger::instance().log(Logger::LogSender::Server, "Error on GET request. Could not parse: " + __receivedGetData,
                           Logger::LogLevel::Error);
    return Json::Value();
}

void Server::changeServerAddress(const std::string& url) {
    this->pause();

    // lock all requests to ensure, all are done and prevent other processes from starting new ones
    std::lock_guard getGuard(this->m_getRequest.lock);
    std::lock_guard postGuard(this->m_postRequest.lock);
    std::lock_guard patchGuard(this->m_patchRequest.lock);
    std::lock_guard deleteGuard(this->m_deleteRequest.lock);

    this->m_baseUrl = url;
    this->m_apiIsChecked = false;
    this->m_apiIsValid = false;

    this->resume();
}

bool Server::checkWebApi() {
    if (this->m_apiIsChecked == true) return this->m_apiIsValid;

    auto response = this->getMessage(this->m_baseUrl + "/api/v1/config/plugin");

    if (response.empty()) {
        Logger::instance().log(Logger::LogSender::Server, "Invalid backend-version response on web API check",
                               Logger::LogLevel::Error);
        Plugin->DisplayMessage("Invalid backend-version response. Check the server URL.");
        this->m_apiIsValid = false;
        return this->m_apiIsValid;
    }

    // check version of plugin matches required version from backend
    Json::Value configJson = response["version"];
    int majorVersion = configJson.get("major", Json::Value(-1)).asInt();
    if (PLUGIN_VERSION_MAJOR != majorVersion) {
        if (majorVersion == -1) {
            Plugin->DisplayMessage("Could not find required major version, confirm the server URL.");
        } else {
            Plugin->DisplayMessage("Backend-version is incompatible. Please update the plugin.");
            Plugin->DisplayMessage("Server requires version " + std::to_string(majorVersion) + ".X.X. ");
            Plugin->DisplayMessage("You are using version " + std::string(PLUGIN_VERSION));
        }
        this->m_apiIsValid = false;
    } else {
        this->m_apiIsValid = true;

        // set config parameters
        ServerConfiguration config;

        Json::Value versionObject = response["version"];
        config.versionFull = versionObject["version"].asString();
        config.versionMajor = versionObject["major"].asInt();
        config.versionMinor = versionObject["minor"].asInt();
        config.versionPatch = versionObject["patch"].asInt();

        Json::Value configObject = response["config"];
        config.name = configObject["serverName"].asString();
        config.allowMasterInSweatbox = configObject["allowSimSession"].asBool();
        config.allowMasterAsObserver = configObject["allowObsMaster"].asBool();

        // get supported airports from backend
        Json::Value airportsJsonArray = response["supportedAirports"];
        if (!airportsJsonArray.empty()) {
            for (size_t i = 0; i < airportsJsonArray.size(); i++) {
                const std::string airport = airportsJsonArray[i].asString();

                config.supportedAirports.push_back(airport);
            }
        }
        this->m_serverConfiguration = config;
    }

    m_apiIsChecked = true;
    return this->m_apiIsValid;
}

const Server::ServerConfiguration Server::getServerConfig() const { return this->m_serverConfiguration; }

const bool JsonHasKey(const Json::Value& json, const std::string& key) { return json.isObject() && json.isMember(key); }

std::list<types::Pilot> Server::getPilots(const std::list<std::string> airports) {
    if (true == this->m_pause || false == this->m_apiIsChecked || false == this->m_apiIsValid) return {};

    std::string url = this->m_baseUrl + "/api/v1/pilots/";
    if (airports.size() != 0) {
        url += "?adep=" +
               std::accumulate(std::next(airports.begin()), airports.end(), airports.front(),
                               [](const std::string& acc, const std::string& str) { return acc + "&adep=" + str; });
    }

    auto response = this->getMessage(url);
    Logger::instance().log(Logger::LogSender::Server,
                           "Performed GET pilots on " + url + ", with response: " + response.toStyledString(),
                           Logger::LogLevel::Info);

    bool pilotsArrayExists = JsonHasKey(response, "pilots");
    if (false == pilotsArrayExists) {
        Logger::instance().log(Logger::LogSender::Server, "GET pilots response has no \"pilots\" key",
                               Logger::LogLevel::Critical);
        return {};
    }

    std::list<types::Pilot> pilots;
    for (const auto& pilot : std::as_const(response["pilots"])) {
        pilots.push_back(types::Pilot());

        pilots.back().callsign = pilot["callsign"].asString();
        pilots.back().lastUpdate = utils::Date::isoStringToTimestamp(pilot["updatedAt"].asString());
        pilots.back().inactive = pilot["inactive"].asBool();

        // position data
        pilots.back().latitude = pilot["position"]["lat"].asDouble();
        pilots.back().longitude = pilot["position"]["lon"].asDouble();
        pilots.back().taxizoneIsTaxiout = pilot["vacdm"]["taxizoneIsTaxiout"].asBool();

        // flightplan & clearance data
        pilots.back().origin = pilot["flightplan"]["adep"].asString();
        pilots.back().destination = pilot["flightplan"]["ades"].asString();
        pilots.back().runway = pilot["clearance"]["dep_rwy"].asString();
        pilots.back().sid = pilot["clearance"]["sid"].asString();

        // ACDM procedure data
        pilots.back().eobt = utils::Date::isoStringToTimestamp(pilot["vacdm"]["eobt"].asString());
        pilots.back().tobt = utils::Date::isoStringToTimestamp(pilot["vacdm"]["tobt"].asString());
        pilots.back().tobt_state = pilot["vacdm"]["tobt_state"].asString();
        pilots.back().ctot = utils::Date::isoStringToTimestamp(pilot["vacdm"]["ctot"].asString());
        pilots.back().ttot = utils::Date::isoStringToTimestamp(pilot["vacdm"]["ttot"].asString());
        pilots.back().tsat = utils::Date::isoStringToTimestamp(pilot["vacdm"]["tsat"].asString());
        pilots.back().exot = std::chrono::utc_clock::time_point(std::chrono::minutes(pilot["vacdm"]["exot"].asInt64()));
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

    return pilots;
}

void Server::sendPostMessage(const std::string& endpointUrl, const Json::Value& root) {
    if (this->m_pause == true || this->m_apiIsChecked == false || this->m_apiIsValid == false ||
        this->m_clientIsMaster == false)
        return;

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
    if (this->m_pause == true || this->m_apiIsChecked == false || this->m_apiIsValid == false ||
        this->m_clientIsMaster == false)
        return;

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
    if (this->m_pause == true || this->m_apiIsChecked == false || this->m_apiIsValid == false ||
        this->m_clientIsMaster == false)
        return;

    Json::StreamWriterBuilder builder{};

    std::lock_guard guard(this->m_deleteRequest.lock);
    if (m_deleteRequest.socket != nullptr) {
        std::string url = m_baseUrl + endpointUrl;

        curl_easy_setopt(m_deleteRequest.socket, CURLOPT_URL, url.c_str());

        curl_easy_perform(m_deleteRequest.socket);
        __receivedDeleteData.clear();
    }
}

void Server::postInitialPilotData(const types::Pilot& pilot) {
    Json::Value root;

    root["callsign"] = pilot.callsign;
    root["inactive"] = false;

    root["position"] = Json::Value();
    root["position"]["lat"] = pilot.latitude;
    root["position"]["lon"] = pilot.longitude;

    root["flightplan"] = Json::Value();
    root["flightplan"]["adep"] = pilot.origin;
    root["flightplan"]["ades"] = pilot.destination;

    root["vacdm"] = Json::Value();
    root["vacdm"]["eobt"] = utils::Date::timestampToIsoString(pilot.eobt);
    root["vacdm"]["tobt"] = utils::Date::timestampToIsoString(pilot.tobt);

    root["clearance"] = Json::Value();
    root["clearance"]["dep_rwy"] = pilot.runway;
    root["clearance"]["sid"] = pilot.sid;

    this->sendPostMessage("/api/v1/pilots", root);
}

void Server::sendCustomDpiTaxioutTime(const std::string& callsign, const std::chrono::utc_clock::time_point& exot) {
    Json::Value message;

    message["callsign"] = callsign;
    message["message_type"] = "X-DPI-taxi";
    message["exot"] = std::chrono::duration_cast<std::chrono::minutes>(exot.time_since_epoch()).count();

    this->sendPatchMessage("/api/v1/messages/x-dpi-t", message);
}

// CONFIRMED TOBT -> TDPI-T
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

void Server::sendTargetDpiTarget(const types::Pilot& data) {
    Json::Value message;

    message["callsign"] = data.callsign;
    message["message_type"] = "T-DPI-t";
    message["tobt_state"] = "CONFIRMED";
    message["tobt"] = utils::Date::timestampToIsoString(data.tobt);

    this->sendPatchMessage("/api/v1/messages/t-dpi-t", message);
}

void Server::sendTargetDpiNow(const types::Pilot& data) {
    Json::Value message;

    message["callsign"] = data.callsign;
    message["message_type"] = "T-DPI-n";
    message["tobt_state"] = data.tobt_state;

    this->sendPatchMessage("/api/v1/messages/t-dpi-n", message);
}

void Server::sendTargetDpiSequenced(const std::string& callsign, const std::chrono::utc_clock::time_point& asat) {
    Json::Value message;

    message["callsign"] = callsign;
    message["message_type"] = "T-DPI-s";
    message["asat"] = utils::Date::timestampToIsoString(asat);

    this->sendPatchMessage("/api/v1/messages/t-dpi-s", message);
}

void Server::sendAtcDpi(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt) {
    Json::Value message;

    message["callsign"] = callsign;
    message["message_type"] = "A-DPI";
    message["aobt"] = utils::Date::timestampToIsoString(aobt);

    this->sendPatchMessage("/api/v1/messages/a-dpi", message);
}

// X-DPI-R
void Server::sendCustomDpiRequest(const std::string& callsign, const std::chrono::utc_clock::time_point& timePoint,
                                  const bool isAsrtUpdate) {
    Json::Value message;

    message["callsign"] = callsign;
    message["message_type"] = "X-DPI-req";
    if (true == isAsrtUpdate)
        message["asrt"] = utils::Date::timestampToIsoString(timePoint);
    else
        message["aort"] = utils::Date::timestampToIsoString(timePoint);

    this->sendPatchMessage("/api/v1/messages/x-dpi-r", message);
}

void Server::sendPilotDisconnect(const std::string& callsign) {
    Json::Value message;

    message["callsign"] = callsign;
    message["inactive"] = true;

    this->sendPatchMessage("/api/v1/messages/inactive", message);
}

// message not yet defined
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

    sendPatchMessage("/api/v1/pilots/" + callsign, root);
}

void Server::deletePilot(const std::string& callsign) { sendDeleteMessage("/api/v1/pilots/" + callsign); }

void Server::setMaster(bool master) { this->m_clientIsMaster = master; }

bool Server::getMaster() { return this->m_clientIsMaster; }

Server& Server::instance() {
    static Server __instance;
    return __instance;
}
