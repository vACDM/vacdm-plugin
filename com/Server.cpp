#include "Server.h"

using namespace vacdm;
using namespace vacdm::com;

static constexpr std::uint8_t ApiMajorVersion = 0;
static constexpr std::uint8_t ApiMinorVersion = 0;
static constexpr std::uint8_t ApiPatchVersion = 1;

static std::string __receivedData;

static std::size_t receiveCurl(void* ptr, std::size_t size, std::size_t nmemb, void* stream) {
    (void)stream;

    std::string serverResult = static_cast<char*>(ptr);
    __receivedData += serverResult;
    return size * nmemb;
}

Server::Server() :
        m_getRequest(),
        m_postRequest(),
        m_patchRequest(),
        m_deleteRequest(),
        m_firstCall(true),
        m_validWebApi(false),
        m_baseUrl("https://vacdm.dotfionn.de/api/v1"),
        m_master(false) {
    /* configure the get request */
    curl_easy_setopt(m_getRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurl);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_TIMEOUT, 2L);

    /* configure the post request */
    curl_easy_setopt(m_postRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_postRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_postRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_postRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurl);
    curl_easy_setopt(m_postRequest.socket, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(m_postRequest.socket, CURLOPT_VERBOSE, 1);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(m_postRequest.socket, CURLOPT_HTTPHEADER, headers);

    /* configure the patch request */
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurl);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTPHEADER, headers);

    /* configure the delete request */
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurl);
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

bool Server::checkWepApi() {
    /* API is already checked */
    if (false == m_firstCall)
        return this->m_validWebApi;

    m_validWebApi = false;

    std::lock_guard guard(m_getRequest.lock);
    if (nullptr != m_getRequest.socket && true == m_firstCall) {
        __receivedData.clear();

        std::string url = m_baseUrl + "/version";
        curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

        /* send the command */
        CURLcode result = curl_easy_perform(m_getRequest.socket);
        if (CURLE_OK == result)
        {
            Json::CharReaderBuilder builder{};
            auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
            std::string errors;
            Json::Value root;

            if (reader->parse(__receivedData.c_str(), __receivedData.c_str() + __receivedData.length(), &root, &errors)) {
                if (ApiMajorVersion != root.get("major", Json::Value(-1)).asInt() || ApiMinorVersion != root.get("minor", Json::Value(-1)).asInt())
                    this->m_validWebApi = false;
                else
                    this->m_validWebApi = true;
            }
            else {
                this->m_validWebApi = false;
            }
        }
    }

    m_firstCall = false;
    return this->m_validWebApi;
}

void Server::setMaster(bool master) {
    this->m_master = master;
}

std::list<types::Flight_t> Server::allFlights(const std::string& airport) {
    if (true == this->m_firstCall || false == this->m_validWebApi)
        return {};

    std::lock_guard guard(m_getRequest.lock);
    if (nullptr != m_getRequest.socket) {
        __receivedData.clear();

        std::string url = m_baseUrl + "/pilots";
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

            if (reader->parse(__receivedData.c_str(), __receivedData.c_str() + __receivedData.length(), &root, &errors) && root.isArray()) {
                std::list<types::Flight_t> flights;

                for (const auto& flight : std::as_const(root)) {
                    flights.push_back(types::Flight_t());

                    flights.back().callsign = flight["callsign"].asString();
                    flights.back().inactive = flight["inactive"].asBool();

                    flights.back().latitude = flight["position"]["lat"].asDouble();
                    flights.back().longitude = flight["position"]["lon"].asDouble();

                    flights.back().origin = flight["flightplan"]["departure"].asString();
                    flights.back().destination = flight["flightplan"]["arrival"].asString();
                    flights.back().rule = flight["flightplan"]["flight_rules"].asString();

                    flights.back().eobt = flight["vacdm"]["eobt"].asString();
                    flights.back().tobt = flight["vacdm"]["tobt"].asString();
                    flights.back().ctot = flight["vacdm"]["ctot"].asString();
                    flights.back().ttot = flight["vacdm"]["ttot"].asString();
                    flights.back().tsat = flight["vacdm"]["tsat"].asString();
                    flights.back().exot = flight["vacdm"]["exot"].asString();

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

    std::lock_guard guard(this->m_postRequest.lock);
    if (nullptr != m_postRequest.socket) {
        std::string url = m_baseUrl + "/pilots";
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

    std::lock_guard guard(this->m_patchRequest.lock);
    if (nullptr != m_patchRequest.socket) {
        std::string url = m_baseUrl + "/pilots/" + callsign;
        curl_easy_setopt(m_patchRequest.socket, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_patchRequest.socket, CURLOPT_POSTFIELDS, message.c_str());

        /* send the command */
        curl_easy_perform(m_patchRequest.socket);
    }
}

Server& Server::instance() {
    static Server __instance;
    return __instance;
}
