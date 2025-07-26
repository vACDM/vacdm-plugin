// tests the messages based on backend pilot model:
// https://github.com/vACDM/vacdm-server/blob/85c9386268fc56edc0d783199bde52a5b02728c2/src/backend/pilot/pilot.dto.ts

#include <gtest/gtest.h>

#include <chrono>

#include "backend/JsonBuilder.h"

using namespace vacdm::backend;

namespace backend::tests {

class JsonBuilderTest : public ::testing::Test {
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

TEST_F(JsonBuilderTest, Test_buildInitialPilotData) {
    const auto json = JsonBuilder::buildInitialPilotData(data);

    // callsign field:
    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    // inactive field:
    ASSERT_TRUE(json.isMember("inactive"));
    ASSERT_TRUE(json["inactive"].isBool());

    // position object:
    ASSERT_TRUE(json.isMember("position"));
    ASSERT_TRUE(json["position"].isObject());
    ASSERT_TRUE(json["position"].isMember("lat"));
    ASSERT_TRUE(json["position"]["lat"].isDouble());
    ASSERT_TRUE(json["position"].isMember("lon"));
    ASSERT_TRUE(json["position"]["lon"].isDouble());

    // flightplan object:
    ASSERT_TRUE(json.isMember("flightplan"));
    ASSERT_TRUE(json["flightplan"].isObject());
    ASSERT_TRUE(json["flightplan"].isMember("adep"));
    ASSERT_TRUE(json["flightplan"]["adep"].isString());
    ASSERT_TRUE(json["flightplan"].isMember("ades"));
    ASSERT_TRUE(json["flightplan"]["ades"].isString());

    // vacdm object:
    ASSERT_TRUE(json.isMember("vacdm"));
    ASSERT_TRUE(json["vacdm"].isObject());
    ASSERT_TRUE(json["vacdm"].isMember("eobt"));
    ASSERT_TRUE(json["vacdm"]["eobt"].isString());
    ASSERT_TRUE(json["vacdm"].isMember("tobt"));
    ASSERT_TRUE(json["vacdm"]["tobt"].isString());

    // clearance object:
    ASSERT_TRUE(json.isMember("clearance"));
    ASSERT_TRUE(json["clearance"].isObject());
    ASSERT_TRUE(json["clearance"].isMember("dep_rwy"));
    ASSERT_TRUE(json["clearance"]["dep_rwy"].isString());
    ASSERT_TRUE(json["clearance"].isMember("sid"));
    ASSERT_TRUE(json["clearance"]["sid"].isString());
}

TEST_F(JsonBuilderTest, Test_buildTargetDpiNow) {
    const auto json = JsonBuilder::buildTargetDpiNow(data);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("messageType"));
    ASSERT_TRUE(json["messageType"].isString());
    ASSERT_EQ(json["messageType"].asString(), "T-DPI-n");

    ASSERT_TRUE(json.isMember("tobtState"));
    ASSERT_TRUE(json["tobtState"].isString());
}

TEST_F(JsonBuilderTest, Test_buildTargetDpiTarget) {
    const auto json = JsonBuilder::buildTargetDpiTarget(data);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("messageType"));
    ASSERT_TRUE(json["messageType"].isString());
    ASSERT_EQ(json["messageType"].asString(), "T-DPI-t");

    ASSERT_TRUE(json.isMember("tobtState"));
    ASSERT_TRUE(json["tobtState"].isString());
    ASSERT_EQ(json["tobtState"].asString(), "CONFIRMED");

    ASSERT_TRUE(json.isMember("tobt"));
    ASSERT_TRUE(json["tobt"].isString());
}

TEST_F(JsonBuilderTest, Test_buildTargetDpiSequenced) {
    const auto json = JsonBuilder::buildTargetDpiSequenced(data);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("messageType"));
    ASSERT_TRUE(json["messageType"].isString());
    ASSERT_EQ(json["messageType"].asString(), "T-DPI-s");

    ASSERT_TRUE(json.isMember("asat"));
    ASSERT_TRUE(json["asat"].isString());
}

TEST_F(JsonBuilderTest, Test_buildAtcDpi) {
    const auto json = JsonBuilder::buildAtcDpi(data);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("messageType"));
    ASSERT_TRUE(json["messageType"].isString());
    ASSERT_EQ(json["messageType"].asString(), "A-DPI");

    ASSERT_TRUE(json.isMember("aobt"));
    ASSERT_TRUE(json["aobt"].isString());
}

TEST_F(JsonBuilderTest, Test_buildCustomDpiTaxioutTime) {
    const auto json = JsonBuilder::buildCustomDpiTaxioutTime(data);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("messageType"));
    ASSERT_TRUE(json["messageType"].isString());
    ASSERT_EQ(json["messageType"].asString(), "X-DPI-taxi");

    ASSERT_TRUE(json.isMember("exot"));
    ASSERT_TRUE(json["exot"].isString());
}

TEST_F(JsonBuilderTest, Test_buildCustomDpiRequest_AsrtUpdate) {
    const auto json = JsonBuilder::buildCustomDpiRequest(data, true);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("messageType"));
    ASSERT_TRUE(json["messageType"].isString());
    ASSERT_EQ(json["messageType"].asString(), "X-DPI-req");

    ASSERT_TRUE(json.isMember("asrt"));
    ASSERT_TRUE(json["asrt"].isString());
}

TEST_F(JsonBuilderTest, Test_buildCustomDpiRequest_AortUpdate) {
    const auto json = JsonBuilder::buildCustomDpiRequest(data, false);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("messageType"));
    ASSERT_TRUE(json["messageType"].isString());
    ASSERT_EQ(json["messageType"].asString(), "X-DPI-req");

    ASSERT_TRUE(json.isMember("aort"));
    ASSERT_TRUE(json["aort"].isString());
}

TEST_F(JsonBuilderTest, Test_buildPilotDisconnect) {
    const auto json = JsonBuilder::buildPilotDisconnect(data.callsign);

    ASSERT_TRUE(json.isMember("callsign"));
    ASSERT_TRUE(json["callsign"].isString());

    ASSERT_TRUE(json.isMember("disconnected"));
    ASSERT_TRUE(json["disconnected"].isBool());
}

}  // namespace backend::tests