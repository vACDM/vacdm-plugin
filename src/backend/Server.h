#pragma once

#define CURL_STATICLIB 1
#include <curl/curl.h>
#include <json/json.h>

#include <list>
#include <mutex>
#include <string>

#include "backend/auth/AuthHandler.h"
#include "types/Pilot.h"

namespace vacdm::com {
class Server {
   public:
    typedef struct ServerConfiguration {
        std::string name = "";
        bool allowMasterInSweatbox = false;
        bool allowMasterAsObserver = false;

        std::string versionFull = "";
        std::int64_t versionMajor = -1;
        std::int64_t versionMinor = -1;
        std::int64_t versionPatch = -1;

        std::list<std::string> supportedAirports;
    } ServerConfiguration_t;

   private:
    friend class AuthHandler;
    Server();
    struct Communication {
        std::mutex lock;
        CURL* socket;

        Communication() : lock(), socket(curl_easy_init()) {}
    };

    static inline struct curl_slist* defaultHeader =
        curl_slist_append(curl_slist_append(nullptr, "Accept: application/json"), "Content-Type: application/json");

    Communication m_getRequest;
    Communication m_postRequest;
    Communication m_patchRequest;
    Communication m_deleteRequest;

    bool m_pause;
    bool m_apiIsChecked;
    bool m_apiIsValid;
    std::string m_baseUrl;
    bool m_clientIsMaster;
    ServerConfiguration m_serverConfiguration;

    void resume();
    void pause();

    const Json::Value getMessage(const std::string& url);
    const Json::Value postMessage(const std::string& url, const Json::Value root);
    const Json::Value patchMessage(const std::string& url, const Json::Value root);
    const Json::Value deleteMessage(const std::string& url, const Json::Value root);

    void setAuthKey(const std::string& authKey);

   public:
    ~Server();
    Server(const Server&) = delete;
    Server(Server&&) = delete;

    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;

    static Server& instance();

    void changeServerAddress(const std::string& url);
    bool checkWebApi();
    const ServerConfiguration getServerConfig() const;
    std::list<types::Pilot> getPilots(const std::list<std::string> airports);

    void updateTobt(const types::Pilot& pilot, const std::chrono::utc_clock::time_point& tobt, bool manualTobt);

    void resetTobt(const std::string& callsign, const std::chrono::utc_clock::time_point& tobt,
                   const std::string& tobtState);
    void deletePilot(const std::string& callsign);

    // messages to backend | DPI = Departure Planing Information

    void patchPilot(const std::string& endpointUrl, const Json::Value& root);
    void postInitialPilotData(const types::Pilot& data);
    void sendTargetDpiNow(const types::Pilot& data);
    void sendTargetDpiTarget(const types::Pilot& data);
    void sendTargetDpiSequenced(const std::string& callsign, const std::chrono::utc_clock::time_point& asat);
    void sendAtcDpi(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt);
    void sendCustomDpiTaxioutTime(const std::string& callsign, const std::chrono::utc_clock::time_point& exot);
    void sendCustomDpiRequest(const std::string& callsign, const std::chrono::utc_clock::time_point& timePoint,
                              const bool isAsrtUpdate);
    void sendPilotDisconnect(const std::string& callsign);

    const std::string& errorMessage() const;
    void setMaster(bool master);
    bool getMaster();
};
}  // namespace vacdm::com