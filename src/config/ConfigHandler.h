#pragma once

#include <mutex>
#include <vector>

#include "config/PluginConfig.h"

namespace vacdm {
class ConfigObserver {
   public:
    virtual void onConfigUpdate(PluginConfig pluginConfig) = 0;
};

/// @brief Responsible for parsing the PluginConfig and updating the respective places where the config is used
class ConfigHandler {
   public:
    PluginConfig subscribe(ConfigObserver *observer);
    void unsubscribe(ConfigObserver *observer);

    static ConfigHandler &instance();

    PluginConfig getConfig();
    void reloadConfig(bool initialLoading = false);

    void changeUrl(std::string &url);
    void changeUpdateCycle(const int updateCycleSeconds);

   private:
    ConfigHandler();
    ~ConfigHandler();
    ConfigHandler(const ConfigHandler &) = delete;
    ConfigHandler(ConfigHandler &&) = delete;

    ConfigHandler &operator=(const ConfigHandler &) = delete;
    ConfigHandler &operator=(ConfigHandler &&) = delete;

    const std::string m_configFileName = "vacdm.txt";
    PluginConfig m_pluginConfig;

    std::mutex m_observersLock;
    std::vector<ConfigObserver *> m_observers;
    void configUpdateEvent();

    PluginConfig parse();
    bool parseColor(const std::string &block, COLORREF &color, std::uint32_t line);

    std::uint32_t m_errorLine;   // Defines the line number the error has occurred
    std::string m_errorMessage;  // The error message to print
};
}  // namespace vacdm