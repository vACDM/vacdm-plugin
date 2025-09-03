#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "PluginConfig.h"

namespace config {
class IConfigObserver {
   public:
    virtual void onConfigChange(const vacdm::PluginConfig& config) = 0;
    virtual ~IConfigObserver() {}
};
}  // namespace config
