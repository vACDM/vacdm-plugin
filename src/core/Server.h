#pragma once

#define CURL_STATICLIB 1
#include <curl/curl.h>
#include <json/json.h>

#include <list>
#include <mutex>
#include <string>

#include "types/Pilot.h"

namespace vacdm::com {
class Server {
   public:
    typedef struct ServerConfiguration_t {
        std::string name = "";
        bool allowMasterInSweatbox = false;
        bool allowMasterAsObserver = false;
    } ServerConfiguration;

   private:
    Server();
    struct Communication {
        std::mutex lock;
        CURL* socket;

        Communication() : lock(), socket(curl_easy_init()) {}
    };

    std::string m_authToken;
    Communication m_getRequest;
    Communication m_postRequest;
    Communication m_patchRequest;
    Communication m_deleteRequest;

    bool m_apiIsChecked;
    bool m_apiIsValid;
    std::string m_baseUrl;
    bool m_clientIsMaster;
    std::string m_errorCode;
    ServerConfiguration m_serverConfiguration;

   public:
    ~Server();
    Server(const Server&) = delete;
    Server(Server&&) = delete;

    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;

    static Server& instance();

    void changeServerAddress(const std::string& url);
    bool checkWebApi();
    ServerConfiguration_t getServerConfig();
    std::list<types::Pilot> getPilots(const std::list<std::string> airports);
    void postPilot(types::Pilot);
    void patchPilot(const Json::Value& root);

    /// @brief Sends a post message to the specififed endpoint url with the root as content
    /// @param endpointUrl endpoint url to send the request to
    /// @param root message content
    void sendPostMessage(const std::string& endpointUrl, const Json::Value& root);

    /// @brief Sends a patch message to the specified endpoint url with the root as content
    /// @param endpointUrl endpoint url to send the request to
    /// @param root message content
    void sendPatchMessage(const std::string& endpointUrl, const Json::Value& root);

    void updateExot(const std::string& pilot, const std::chrono::utc_clock::time_point& exot);
    void updateTobt(const types::Pilot& pilot, const std::chrono::utc_clock::time_point& tobt, bool manualTobt);
    void updateAsat(const std::string& callsign, const std::chrono::utc_clock::time_point& asat);
    void updateAsrt(const std::string& callsign, const std::chrono::utc_clock::time_point& asrt);
    void updateAobt(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt);
    void updateAort(const std::string& callsign, const std::chrono::utc_clock::time_point& aort);

    const std::string& errorMessage() const;
    void setMaster(bool master);
    bool getMaster();
};
}  // namespace vacdm::com