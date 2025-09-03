#pragma once

#include <string>
#include <vector>

#include "IConfigHandler.h"
#include "config/PluginConfig.h"

namespace config {
class ConfigHandler : public interfaces::IConfigHandler {
   private:
    std::string url_;
    std::vector<std::weak_ptr<config::IConfigObserver>> observers_;
    vacdm::PluginConfig config_;

   public:
    ConfigHandler();
    ~ConfigHandler();

    void subscribe(std::shared_ptr<config::IConfigObserver> obs) override;
    void unsubscribe(std::shared_ptr<config::IConfigObserver> obs) override;
    void notify() override;

    void changeUrl(const std::string& url) override;
};
}  // namespace config
