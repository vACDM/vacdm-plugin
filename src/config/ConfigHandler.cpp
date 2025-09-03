#include "ConfigHandler.h"

using namespace config;

ConfigHandler::ConfigHandler() {}

ConfigHandler::~ConfigHandler() {}

void config::ConfigHandler::subscribe(std::shared_ptr<config::IConfigObserver> obs) { observers_.push_back(obs); }

void config::ConfigHandler::unsubscribe(std::shared_ptr<config::IConfigObserver> obs) {
    observers_.erase(std::remove_if(observers_.begin(), observers_.end(),
                                    [&obs](const std::weak_ptr<config::IConfigObserver>& wptr) {
                                        auto sp = wptr.lock();
                                        return !sp || sp == obs;  // remove if expired or matches
                                    }),
                     observers_.end());
}

void config::ConfigHandler::notify() {
    for (auto it = observers_.begin(); it != observers_.end();) {
        if (auto obs = it->lock()) {
            obs->onConfigChange(config_);
            ++it;
        } else {
            it = observers_.erase(it);  // clean up expired
        }
    }
}

void config::ConfigHandler::changeUrl(const std::string& url) {
    // TODO: url validation ?
    url_ = url;
}
