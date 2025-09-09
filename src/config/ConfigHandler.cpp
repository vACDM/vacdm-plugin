#include "ConfigHandler.h"

#include "core/DataManager.h"
#include "utils/Colors.h"
#include "utils/String.h"

using namespace config;

ConfigHandler::ConfigHandler(const std::filesystem::path& path) : config_path_(path) {}

ConfigHandler::~ConfigHandler() {}

void ConfigHandler::subscribe(std::shared_ptr<config::IConfigObserver> obs) { observers_.push_back(obs); }

void ConfigHandler::unsubscribe(std::shared_ptr<config::IConfigObserver> obs) {
    observers_.erase(std::remove_if(observers_.begin(), observers_.end(),
                                    [&obs](const std::weak_ptr<config::IConfigObserver>& wptr) {
                                        auto sp = wptr.lock();
                                        return !sp || sp == obs;  // remove if expired or matches
                                    }),
                     observers_.end());
}

void ConfigHandler::notify() {
    for (auto it = observers_.begin(); it != observers_.end();) {
        if (auto obs = it->lock()) {
            obs->onConfigChange(config_);
            ++it;
        } else {
            it = observers_.erase(it);  // clean up expired
        }
    }
}

vacdm::PluginConfig ConfigHandler::getConfig() { return config_; }

void ConfigHandler::changeUrl(const std::string& url) {
    // TODO: url validation ?
    url_ = url;
}

void ConfigHandler::registerPluginDisplayMessageCallback(
    std::function<void(const std::string&, const std::string&)> cb) {
    displayMessageCallback_ = std::move(cb);
}

void ConfigHandler::sendCallbackMessage(const std::string& msg) {
    if (displayMessageCallback_) {
        displayMessageCallback_(msg, "ConfigHandler");
    }
}

void ConfigHandler::load(bool initialLoading) {
    vacdm::PluginConfig config;
    std::ifstream stream(config_path_);
    if (!stream.is_open()) {
        sendCallbackMessage("Could not open config file: " + config_path_.string());
        return;
    }

    // mapping for all color configuration entries
    using ColorTarget = COLORREF vacdm::PluginConfig::*;
    static const std::unordered_map<std::string, ColorTarget> colorMap = {
        {"COLOR_lightgreen", &vacdm::PluginConfig::lightgreen},
        {"COLOR_lightblue", &vacdm::PluginConfig::lightblue},
        {"COLOR_green", &vacdm::PluginConfig::green},
        {"COLOR_blue", &vacdm::PluginConfig::blue},
        {"COLOR_lightyellow", &vacdm::PluginConfig::lightyellow},
        {"COLOR_yellow", &vacdm::PluginConfig::yellow},
        {"COLOR_orange", &vacdm::PluginConfig::orange},
        {"COLOR_red", &vacdm::PluginConfig::red},
        {"COLOR_grey", &vacdm::PluginConfig::grey},
        {"COLOR_white", &vacdm::PluginConfig::white},
        {"COLOR_debug", &vacdm::PluginConfig::debug}};

    std::string line;
    std::uint32_t lineOffset = 0;

    while (std::getline(stream, line)) {
        ++lineOffset;

        std::string trimmed = vacdm::utils::String::trim(line);
        // skip empty lines
        if (trimmed.empty()) continue;

        // trim the line and skip comments
        if (trimmed[0] == '#') continue;

        // split key=value
        std::vector<std::string> values = vacdm::utils::String::splitString(trimmed, "=");
        if (values.size() != 2 || values[1].empty()) {
            sendCallbackMessage("Invalid configuration entry at line " + std::to_string(lineOffset));
            return;
        }

        const std::string& key = values[0];
        const std::string& value = values[1];
        bool parsed = false;
        std::string colorParseError;

        if (key == "SERVER_url") {
            config.serverUrl = value;
            parsed = true;
        } else if (key == "UPDATE_RATE_SECONDS") {
            try {
                const int updateCycleSeconds = std::stoi(value);
                if (updateCycleSeconds < core::minUpdateCycleSeconds ||
                    updateCycleSeconds > core::maxUpdateCycleSeconds) {
                    sendCallbackMessage("UPDATE_RATE_SECONDS must be between " +
                                        std::to_string(core::minUpdateCycleSeconds) + " and " +
                                        std::to_string(core::maxUpdateCycleSeconds));
                } else {
                    config.updateCycleSeconds = updateCycleSeconds;
                    parsed = true;
                }
            } catch (const std::exception& e) {
                sendCallbackMessage("Error while parsing UPDATE_RATE_SECONDS: " + std::string(e.what()));
            }
        } else {
            // look up colors
            auto it = colorMap.find(key);
            if (it != colorMap.end()) {
                parsed = ::utils::colors::parseColor(value, config.*(it->second), lineOffset, colorParseError);
            } else {
                sendCallbackMessage("Unknown configuration entry '" + key + "' at line " + std::to_string(lineOffset));
                return;
            }
        }

        // handle parse failure
        if (!parsed) {
            if (!colorParseError.empty()) {
                sendCallbackMessage("Failed to parse color entry at line " + std::to_string(lineOffset) + ": " +
                                    colorParseError);
            } else {
                sendCallbackMessage("Failed to parse entry at line " + std::to_string(lineOffset));
            }
            return;
        }
    }

    config.valid = true;
    sendCallbackMessage("loaded config successfully");
}