#pragma once

#include <functional>
#include <optional>

#include "IConfigObserver.h"
#include "PluginConfig.h"

namespace interfaces {
class IConfigHandler {
   public:
    virtual ~IConfigHandler() = default;

    virtual void registerPluginDisplayMessageCallback(
        std::function<void(const std::string &, const std::string &)> cb) = 0;

    virtual void subscribe(std::shared_ptr<config::IConfigObserver> obs) = 0;
    virtual void unsubscribe(std::shared_ptr<config::IConfigObserver> obs) = 0;
    virtual void notify() = 0;

    virtual vacdm::PluginConfig getConfig() = 0;

    virtual void changeUrl(const std::string &url) = 0;
    virtual void load(bool initialLoading) = 0;
};
}  // namespace interfaces
