#include "ConfigHandler.h"

#include <fstream>
#include <numeric>

#include "backend/Server.h"
#include "core/DataManager.h"
#include "handlers/FileHandler.h"
#include "log/Logger.h"
#include "main.h"
#include "utils/String.h"
#include "vACDM.h"


using namespace vacdm;
using namespace vacdm::logging;

ConfigHandler::ConfigHandler() { this->reloadConfig(true); }

ConfigHandler::~ConfigHandler() {}

ConfigHandler& ConfigHandler::instance() {
    static ConfigHandler instance;
    return instance;
}

PluginConfig ConfigHandler::subscribe(ConfigObserver* observer) {
    std::lock_guard guard(this->m_observersLock);
    this->m_observers.push_back(observer);
    return this->m_pluginConfig;
}

void ConfigHandler::unsubscribe(ConfigObserver* observer) {
    std::lock_guard guard(this->m_observersLock);
    auto it = std::find(this->m_observers.begin(), this->m_observers.end(), observer);
    if (it != this->m_observers.end()) {
        this->m_observers.erase(it);
    }
}

void ConfigHandler::configUpdateEvent() {
    // save the config
    handlers::FileHandler::saveFile(configToString(this->m_pluginConfig),
                                    handlers::FileHandler::getDllPath() + "\\" + this->m_configFileName);

    // send new config to subscribers
    std::lock_guard guard(this->m_observersLock);
    for (ConfigObserver* observer : this->m_observers) {
        observer->onConfigUpdate(this->m_pluginConfig);
    }
}

void ConfigHandler::reloadConfig(bool initialLoading) {
    PluginConfig newConfig = this->parse();

    if (false == newConfig.valid) {
        const std::string message = std::to_string(this->m_errorLine) + ": " + this->m_errorMessage + " Line: ";
        Logger::instance().log(Logger::LogSender::ConfigHandler, message, Logger::LogLevel::Error);
        Plugin->DisplayMessage(message, "Config");
        return;
    }

    this->changeUrl(newConfig.serverUrl);

    this->m_pluginConfig = newConfig;

    const std::string message = true == initialLoading ? "Loaded the config" : "Reloaded the config";
    Logger::instance().log(Logger::LogSender::ConfigHandler,
                           message + " Config: " + configToString(this->m_pluginConfig), Logger::LogLevel::Info);
    Plugin->DisplayMessage(message, "Config");

    this->configUpdateEvent();
}

PluginConfig ConfigHandler::parse() {
    PluginConfig config;
    config.valid = false;

    std::ifstream stream(handlers::FileHandler::getDllPath() + "\\" + this->m_configFileName);
    if (false == stream.is_open()) {
        this->m_errorMessage = "Unable to open the configuration file";
        this->m_errorLine = 0;
        return config;
    }

    std::string line;
    std::uint32_t lineOffset = 0;
    while (std::getline(stream, line)) {
        std::string value;

        lineOffset += 1;

        // skip empty lines
        if (line.length() == 0) continue;

        // trim the line and skip comments
        std::string trimmed = utils::String::trim(line);
        if (trimmed.find_first_of('#', 0) == 0) continue;

        // get values of line
        std::vector<std::string> values = utils::String::splitString(trimmed, "=");
        if (values.size() != 2) {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Invalid configuration entry";
            return config;
        } else {
            value = values[1];
        }

        // end on invalid lines
        if (0 == value.length()) {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Invalid entry";
            return config;
        }

        bool parsed = false;
        if ("SERVER_url" == values[0]) {
            config.serverUrl = values[1];
            parsed = true;
        } else if ("UPDATE_RATE_SECONDS" == values[0]) {
            try {
                const int updateCycleSeconds = std::stoi(values[1]);
                if (updateCycleSeconds < core::minUpdateCycleSeconds ||
                    updateCycleSeconds > core::maxUpdateCycleSeconds) {
                    this->m_errorLine = lineOffset;
                    this->m_errorMessage = "Value must be number between " +
                                           std::to_string(core::minUpdateCycleSeconds) + " and " +
                                           std::to_string(core::maxUpdateCycleSeconds);
                    return config;
                } else {
                    config.updateCycleSeconds = updateCycleSeconds;
                    parsed = true;
                }
            } catch (const std::exception& e) {
                this->m_errorMessage = e.what();
                this->m_errorLine = lineOffset;
                return config;
            }

        } else if ("COLOR_lightgreen" == values[0]) {
            parsed = this->parseColor(values[1], config.lightgreen, lineOffset);
        } else if ("COLOR_lightblue" == values[0]) {
            parsed = this->parseColor(values[1], config.lightblue, lineOffset);
        } else if ("COLOR_green" == values[0]) {
            parsed = this->parseColor(values[1], config.green, lineOffset);
        } else if ("COLOR_blue" == values[0]) {
            parsed = this->parseColor(values[1], config.blue, lineOffset);
        } else if ("COLOR_lightyellow" == values[0]) {
            parsed = this->parseColor(values[1], config.lightyellow, lineOffset);
        } else if ("COLOR_yellow" == values[0]) {
            parsed = this->parseColor(values[1], config.yellow, lineOffset);
        } else if ("COLOR_orange" == values[0]) {
            parsed = this->parseColor(values[1], config.orange, lineOffset);
        } else if ("COLOR_red" == values[0]) {
            parsed = this->parseColor(values[1], config.red, lineOffset);
        } else if ("COLOR_grey" == values[0]) {
            parsed = this->parseColor(values[1], config.grey, lineOffset);
        } else if ("COLOR_white" == values[0]) {
            parsed = this->parseColor(values[1], config.white, lineOffset);
        } else if ("COLOR_debug" == values[0]) {
            parsed = this->parseColor(values[1], config.debug, lineOffset);
        } else {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Unknown file entry: " + value[0];
            return config;
        }

        if (false == parsed) {
            return config;
        }
    }

    config.valid = true;

    return config;
}

bool ConfigHandler::parseColor(const std::string& block, COLORREF& color, std::uint32_t line) {
    std::vector<std::string> colorValues = utils::String::splitString(block, ",");

    if (3 != colorValues.size()) {
        this->m_errorLine = line;
        this->m_errorMessage = "Invalid color config";
        return false;
    }

    std::array<std::uint8_t, 3> colors;
    for (std::size_t i = 0; i < 3; ++i) colors[i] = static_cast<std::uint8_t>(std::atoi(colorValues[i].c_str()));

    color = RGB(colors[0], colors[1], colors[2]);

    return true;
}

void ConfigHandler::changeUrl(std::string& url) {
    auto sanitizedUrl = utils::String::sanitizeUrl(url);

    if (sanitizedUrl == this->m_pluginConfig.serverUrl) return;

    if (sanitizedUrl != url) {
        Logger::instance().log(Logger::LogSender::ConfigHandler, "Sanitized URL " + url + " to " + sanitizedUrl,
                               Logger::LogLevel::Info);
        Plugin->DisplayMessage("Sanitized URL " + url + " to " + sanitizedUrl, "Config");
    }

    com::Server::instance().changeServerAddress(sanitizedUrl);

    const bool apiIsValid = com::Server::instance().checkWebApi();

    if (apiIsValid) {
        this->m_pluginConfig.serverUrl = sanitizedUrl;
        Plugin->DisplayMessage("Server URL " + sanitizedUrl + " set.", "Config");
        Logger::instance().log(Logger::LogSender::ConfigHandler, "Changed URL to " + sanitizedUrl,
                               Logger::LogLevel::Info);
        url = sanitizedUrl;

        std::string serverName = com::Server::instance().getServerConfig().name;
        std::list<std::string> supportedAirports = com::Server::instance().getServerConfig().supportedAirports;
        Plugin->DisplayMessage(("Connected to " + serverName), "Server");
        Plugin->DisplayMessage(
            "ACDM available for: " +
                std::accumulate(std::next(supportedAirports.begin()), supportedAirports.end(),
                                supportedAirports.front(),
                                [](const std::string& acc, const std::string& str) { return acc + " " + str; }),
            "Server");
    } else {
        Plugin->DisplayMessage(
            "Server URL " + sanitizedUrl + " is not valid. Reverting to " + this->m_pluginConfig.serverUrl, "Config");
        com::Server::instance().changeServerAddress(m_pluginConfig.serverUrl);  // revert URL
        Logger::instance().log(Logger::LogSender::ConfigHandler, "URL was not valid: " + sanitizedUrl,
                               Logger::LogLevel::Info);
    }
}

void ConfigHandler::changeUpdateCycle(const int updateCycleSeconds) {
    if (updateCycleSeconds < core::minUpdateCycleSeconds || updateCycleSeconds > core::maxUpdateCycleSeconds) {
        Plugin->DisplayMessage("Could not set update rate", "vACDM");
        return;
    }

    Plugin->DisplayMessage("vACDM updating every " +
                               (updateCycleSeconds == 1 ? "second" : std::to_string(updateCycleSeconds) + " seconds"),
                           "vACDM");
    this->m_pluginConfig.updateCycleSeconds = updateCycleSeconds;
    this->configUpdateEvent();
}