#pragma once

#include "IConfigObserver.h"

namespace interfaces {
class IConfigHandler {
   public:
    virtual ~IConfigHandler() = default;

    virtual void subscribe(std::shared_ptr<config::IConfigObserver> obs) = 0;
    virtual void unsubscribe(std::shared_ptr<config::IConfigObserver> obs) = 0;
    virtual void notify() = 0;

    virtual void changeUrl(const std::string& url) = 0;
};
}  // namespace interfaces
