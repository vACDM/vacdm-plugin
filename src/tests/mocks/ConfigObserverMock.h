#pragma once

#include <gmock/gmock.h>

#include "config/IConfigObserver.h"
#include "config/PluginConfig.h"

namespace mocks {

class MockConfigObserver : public config::IConfigObserver {
   public:
    MOCK_METHOD(void, onConfigChange, (const vacdm::PluginConfig& config), (override));
};

}  // namespace mocks
