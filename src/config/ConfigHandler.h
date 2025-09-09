#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "IConfigHandler.h"
#include "config/PluginConfig.h"
#include "utils/File.h"

namespace config {
class ConfigHandler : public interfaces::IConfigHandler {
   private:
    std::string url_;
    std::vector<std::weak_ptr<config::IConfigObserver>> observers_;
    vacdm::PluginConfig config_;
    std::filesystem::path config_path_;

    std::function<void(const std::string &, const std::string &)> displayMessageCallback_;

   public:
    explicit ConfigHandler(const std::filesystem::path &path = ::utils::file::GetDllDirectoryPathFs());
    ~ConfigHandler();

    void registerPluginDisplayMessageCallback(
        std::function<void(const std::string &, const std::string &)> cb) override;
    void sendCallbackMessage(const std::string &msg);

    void subscribe(std::shared_ptr<config::IConfigObserver> obs) override;
    void unsubscribe(std::shared_ptr<config::IConfigObserver> obs) override;
    void notify() override;

    vacdm::PluginConfig getConfig() override;

    void changeUrl(const std::string &url) override;
    void load(bool initialLoading) override;
};
}  // namespace config
