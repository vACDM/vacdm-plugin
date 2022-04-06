#pragma once

#include <mutex>
#include <list>

#define CURL_STATICLIB 1
#include <curl/curl.h>
#include <json/json.h>

#include <types/Flight.h>

namespace vacdm {
namespace com {

/**
 * @brief Defines the communication with the vACDM backend
 */
class Server {
private:
    struct Communication
    {
        std::mutex lock;
        CURL*      socket;

        Communication() : lock(), socket(curl_easy_init()) { }
    };

    Communication m_getRequest;
    Communication m_postRequest;
    Communication m_patchRequest;
    Communication m_deleteRequest;
    bool          m_firstCall;
    bool          m_validWebApi;
    std::string   m_baseUrl;
    bool          m_master;

    Server();

public:
    ~Server();
    Server(const Server&) = delete;
    Server(Server&&) = delete;

    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;

    bool checkWepApi();
    std::list<types::Flight_t> allFlights(const std::string& airport = "");
    void postFlight(const Json::Value& root);
    void patchFlight(const std::string& callsign, const Json::Value& root);
    void setMaster(bool master);

    /**
     * @brief Returns the current server instance
     * @return The server instance
     */
    static Server& instance();
};

}
}
