#include <gtest/gtest.h>

#include <memory>

#include "api/CurlRestClient.h"

using namespace api;

class CurlRestClientTest : public ::testing::Test {
   protected:
    void SetUp() override {
        client = std::make_unique<CurlRestClient>(nullptr);
        baseUrl = "http://localhost:8080";
    }

    std::unique_ptr<CurlRestClient> client;
    std::string baseUrl;
};

TEST_F(CurlRestClientTest, GetRequest) {
    auto response = client->get(baseUrl + "/get");
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_NE(response.body.find("\"method\": \"GET\""), std::string::npos);
}

TEST_F(CurlRestClientTest, PostRequest) {
    auto response = client->post(baseUrl + "/post", R"({"test":"data"})");
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_NE(response.body.find("\"method\": \"POST\""), std::string::npos);
}

TEST_F(CurlRestClientTest, PatchRequest) {
    auto response = client->patch(baseUrl + "/patch", R"({"test":"data"})");
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_NE(response.body.find("\"method\": \"PATCH\""), std::string::npos);
}

TEST_F(CurlRestClientTest, PutRequest) {
    auto response = client->put(baseUrl + "/put", R"({"test":"data"})");
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_NE(response.body.find("\"method\": \"PUT\""), std::string::npos);
}

TEST_F(CurlRestClientTest, DeleteRequest) {
    auto response = client->del(baseUrl + "/delete");
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_NE(response.body.find("\"method\": \"DELETE\""), std::string::npos);
}
