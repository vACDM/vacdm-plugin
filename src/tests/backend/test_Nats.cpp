#include <gtest/gtest.h>

#include "backend/BackendNats.h"

using namespace vacdm::backend;

namespace backend::tests {

class NatsTest : public ::testing::Test {
   protected:
    vacdm::types::Pilot data;

    void SetUp() override {
        data.callsign = "DLH123";
        data.latitude = 50.0;
        data.longitude = 8.0;
        data.origin = "EDDF";
        data.destination = "EGLL";

        auto eobt = std::chrono::utc_clock::from_sys(std::chrono::sys_days{std::chrono::year{2025} / 7 / 23} +
                                                     std::chrono::hours{12} + std::chrono::minutes{12});
        auto tobt = eobt + std::chrono::minutes{10};

        data.eobt = eobt;
        data.tobt = tobt;
        data.inactive = false;
        data.runway = "25C";
        data.sid = "TOBAK6S";
    }
};

TEST_F(NatsTest, Test_init) {
    auto test = BackendNats("localhost:4222");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    test.postInitialPilotData(data);
}
}  // namespace backend::tests