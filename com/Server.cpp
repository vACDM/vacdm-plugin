#include "Server.h"
#include "Version.h"
#include "logging/Logger.h"

using namespace vacdm;
using namespace vacdm::com;

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

Server::Server() :
        m_authToken(),
        m_getRequest(),
        m_postRequest(),
        m_patchRequest(),
        m_deleteRequest(),
        m_firstCall(true),
        m_validWebApi(false),
        m_baseUrl("https://vacdm.vatsim-germany.org"),
        m_master(false),
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
    this->m_validWebApi = false;
    this->m_firstCall = true;
}

bool Server::checkWebApi() {
    /* API is already checked */
    if (false == this->m_firstCall)
        return this->m_validWebApi;

    std::lock_guard guard(m_getRequest.lock);
    if (nullptr != m_getRequest.socket) {
        __receivedGetData.clear();

        std::string url = m_baseUrl + "/api/v1/config/plugin";
        curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

        /* send the command */
        CURLcode result = curl_easy_perform(m_getRequest.socket);
        if (CURLE_OK == result) {
            Json::CharReaderBuilder builder{};
            auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
            std::string errors;
            Json::Value root;

            logging::Logger::instance().log("Server", logging::Logger::Level::System, "Received configuration: " + __receivedGetData);
            if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root, &errors)) {
                // check version of plugin matches required version from backend
                Json::Value configJson = root["version"];
                if (PLUGIN_VERSION_MAJOR != configJson.get("major", Json::Value(-1)).asInt()) {
                    this->m_errorCode = "Backend-version is incompatible. Please update the plugin.";
                    this->m_validWebApi = false;
                }
                else {
                    this->m_validWebApi = true;

                    // set config parameters
                    ServerConfiguration_t config;
                    config.name = root["config"]["serverName"].asString();
                    config.masterInSweatbox = root["config"]["allowSimSession"].asBool();
                    config.masterAsObserver = root["config"]["allowObsMaster"].asBool();

                    // get supported airports from backend
                    Json::Value airportsJsonArray = root["supportedAirports"];
                    if (!airportsJsonArray.empty()) {
                        for (size_t i = 0; i < airportsJsonArray.size(); i++) {
                            const std::string airport = airportsJsonArray[i].asString();

                            config.backendSupportedAirports.push_back(airport);

                        }
                    }
                    this->m_serverConfiguration = config;
                }
            }
            else {
                this->m_errorCode = "Invalid backend-version response: " + __receivedGetData;
                this->m_validWebApi = false;
            }
        }
    }
    m_firstCall = false;
    return this->m_validWebApi;
}

Server::ServerConfiguration_t Server::serverConfiguration() {
    return this->m_serverConfiguration;
}

void Server::setMaster(bool master) {
    this->m_master = master;
}

std::chrono::utc_clock::time_point Server::isoStringToTimestamp(const std::string& timestamp) {
    std::chrono::utc_clock::time_point retval;
    std::stringstream stream;

    stream << timestamp.substr(0, timestamp.length() - 1);
    std::chrono::from_stream(stream, "%FT%T", retval);

    return retval;
}

std::list<types::Flight_t> Server::allFlights(const std::string& airport) {
    if (true == this->m_firstCall || false == this->m_validWebApi)
        return {};

    std::lock_guard guard(m_getRequest.lock);
    if (nullptr != m_getRequest.socket) {
        __receivedGetData.clear();

        std::string url = m_baseUrl + "/api/v1/pilots";
        if (airport.length() != 0)
            url += "?adep=" + airport;
        curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

        /* send the command */
        CURLcode result = curl_easy_perform(m_getRequest.socket);
        if (CURLE_OK == result) {
            Json::CharReaderBuilder builder{};
            auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
            std::string errors;
            Json::Value root;

            logging::Logger::instance().log("Server", logging::Logger::Level::Debug, "Airport update: " + url);
            logging::Logger::instance().log("Server", logging::Logger::Level::Debug, __receivedGetData);
            if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root, &errors) && root.isArray()) {
                std::list<types::Flight_t> flights;

                for (const auto& flight : std::as_const(root)) {
                    flights.push_back(types::Flight_t());

                    flights.back().lastUpdate = Server::isoStringToTimestamp(flight["updatedAt"].asString());
                    flights.back().callsign = flight["callsign"].asString();
                    flights.back().inactive = flight["inactive"].asBool();

                    flights.back().latitude = flight["position"]["lat"].asDouble();
                    flights.back().longitude = flight["position"]["lon"].asDouble();

                    flights.back().origin = flight["flightplan"]["departure"].asString();
                    flights.back().destination = flight["flightplan"]["arrival"].asString();
                    flights.back().rule = flight["flightplan"]["flight_rules"].asString();

                    flights.back().eobt = Server::isoStringToTimestamp(flight["vacdm"]["eobt"].asString());
                    flights.back().tobt = Server::isoStringToTimestamp(flight["vacdm"]["tobt"].asString());
                    flights.back().ctot = Server::isoStringToTimestamp(flight["vacdm"]["ctot"].asString());
                    flights.back().ttot = Server::isoStringToTimestamp(flight["vacdm"]["ttot"].asString());
                    flights.back().tsat = Server::isoStringToTimestamp(flight["vacdm"]["tsat"].asString());
                    flights.back().asat = Server::isoStringToTimestamp(flight["vacdm"]["asat"].asString());
                    flights.back().aobt = Server::isoStringToTimestamp(flight["vacdm"]["aobt"].asString());
                    flights.back().atot = Server::isoStringToTimestamp(flight["vacdm"]["atot"].asString());
                    flights.back().exot = std::chrono::utc_clock::time_point(std::chrono::minutes(flight["vacdm"]["exot"].asInt64()));
                    flights.back().asrt = Server::isoStringToTimestamp(flight["vacdm"]["asrt"].asString());
                    flights.back().aort = Server::isoStringToTimestamp(flight["vacdm"]["aort"].asString());
                    flights.back().ctot = Server::isoStringToTimestamp(flight["vacdm"]["ctot"].asString());
                    flights.back().tobt_state = flight["vacdm"]["tobt_state"].asString();

                    // ecfmp
                    Json::Value measuresArray = flight["measures"];
                    std::vector<vacdm::types::Measure> parsedMeasures;
                    if (!measuresArray.empty()) {
                        for (size_t i = 0; i < measuresArray.size(); i++) {
                            vacdm::types::Measure measure;
                            // Extract the ident and value fields from the JSON object
                            measure.ident = measuresArray[i]["ident"].asString();
                            measure.value = measuresArray[i]["value"].asInt();

                            // Add the measure struct to the vector
                            parsedMeasures.push_back(measure);

                            logging::Logger::instance().log("JSON", logging::Logger::Level::Debug, "Values: " + measure.ident + std::to_string(measure.value));
                        }
                        if (!parsedMeasures.empty()) {
                            flights.back().measures = parsedMeasures;
                        }
                    }
                    else {
                        logging::Logger::instance().log("JSON", logging::Logger::Level::Debug, "Values: empty");
                    }

                    flights.back().hasBooking = flight["hasBooking"].asBool();
                    
                    flights.back().runway = flight["clearance"]["dep_rwy"].asString();
                    flights.back().sid = flight["clearance"]["sid"].asString();
                    flights.back().assignedSquawk = flight["clearance"]["assigned_squawk"].asString();
                    flights.back().currentSquawk = flight["clearance"]["current_squawk"].asString();
                    flights.back().initialClimb = flight["clearance"]["initial_climb"].asString();
                }

                return flights;
            }
        }
    }

    return {};
}

void Server::postFlight(const Json::Value& root) {
    if (true == this->m_firstCall || false == this->m_validWebApi || false == this->m_master)
        return;

    Json::StreamWriterBuilder builder{};
    const auto message = Json::writeString(builder, root);
    logging::Logger::instance().log("Server", logging::Logger::Level::Debug, "POST: " + message);

    std::lock_guard guard(this->m_postRequest.lock);
    if (nullptr != m_postRequest.socket) {
        std::string url = m_baseUrl + "/api/v1/pilots";
        curl_easy_setopt(m_postRequest.socket, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_postRequest.socket, CURLOPT_POSTFIELDS, message.c_str());

        /* send the command */
        curl_easy_perform(m_postRequest.socket);
    }
}

void Server::patchFlight(const std::string& callsign, const Json::Value& root) {
    if (true == this->m_firstCall || false == this->m_validWebApi || false == this->m_master)
        return;

    Json::StreamWriterBuilder builder{};
    const auto message = Json::writeString(builder, root);
    logging::Logger::instance().log("Server", logging::Logger::Level::Debug, "PATCH: " + message);

    std::lock_guard guard(this->m_patchRequest.lock);
    if (nullptr != m_patchRequest.socket) {
        std::string url = m_baseUrl + "/api/v1/pilots/" + callsign;
        curl_easy_setopt(m_patchRequest.socket, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_patchRequest.socket, CURLOPT_POSTFIELDS, message.c_str());

        /* send the command */
        curl_easy_perform(m_patchRequest.socket);
    }
}

void Server::deleteFlight(const std::string& callsign) {
    if (true == this->m_firstCall || false == this->m_validWebApi || false == this->m_master)
        return;

    Json::StreamWriterBuilder builder{};

    logging::Logger::instance().log("Server", logging::Logger::Level::Debug, "DELETE: " + callsign);

    std::lock_guard guard(this->m_deleteRequest.lock);
    if (m_deleteRequest.socket != nullptr) {
        std::string url = m_baseUrl + "/api/v1/pilots/" + callsign;

        curl_easy_setopt(m_deleteRequest.socket, CURLOPT_URL, url.c_str());

        /* send the command */
        curl_easy_perform(m_deleteRequest.socket);
    }
}

const std::string& Server::errorMessage() const {
    return this->m_errorCode;
}

Server& Server::instance() {
    static Server __instance;
    return __instance;
}
