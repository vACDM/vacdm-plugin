#include <gtest/gtest.h>

#include "backend/BackendNats.h"

using namespace vacdm::backend;

namespace backend::tests {
TEST(NatsTest, Test_init) {
    auto test = BackendNats("localhost:4222");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto test2 = BackendNats("localhost:4222");
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
}  // namespace backend::tests