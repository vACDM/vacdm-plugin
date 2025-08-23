#include <gtest/gtest.h>

#include "backend/Backend.h"
#include "backend/IBackendInterface.h"

using namespace vacdm::backend;

namespace backend::tests {
TEST(BackendTest, Test_init) {
    BackendWebsocket backend("ws://localhost:8081");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    backend.send();
}
}  // namespace backend::tests