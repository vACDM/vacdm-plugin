#pragma once

#include <string>
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
public:
    typedef struct ServerConfiguration {
        std::string name = "";
        bool masterInSweatbox = false;
        bool masterAsObserver = false;
    } ServerConfiguration_t;

private:
    struct Communication
    {
        std::mutex lock;
        CURL*      socket;

        Communication() : lock(), socket(curl_easy_init()) { }
    };

    std::string   m_authToken;
    Communication m_getRequest;
    Communication m_postRequest;
    Communication m_patchRequest;
    Communication m_deleteRequest;
    bool          m_firstCall;
    bool          m_validWebApi;
    std::string   m_baseUrl;
    bool          m_master;
    std::string   m_errorCode;

    Server();

public:
    ~Server();
    Server(const Server&) = delete;
    Server(Server&&) = delete;

    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;

    bool checkWepApi();
    ServerConfiguration_t serverConfiguration();
    std::list<types::Flight_t> allFlights(const std::string& airport = "");
    void postFlight(const Json::Value& root);
    void patchFlight(const std::string& callsign, const Json::Value& root);
    void setMaster(bool master);
    const std::string& errorMessage() const;

    /**
     * @brief Returns the current server instance
     * @return The server instance
     */
    static Server& instance();
};

}
}
