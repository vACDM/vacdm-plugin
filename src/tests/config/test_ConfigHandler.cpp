#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "config/ConfigHandler.h"
#include "config/IConfigHandler.h"
#include "tests/mocks/ConfigObserverMock.h"

class ConfigHandlerTest : public ::testing::Test {
   protected:
    std::shared_ptr<config::ConfigHandler> handler;
    std::vector<std::string> messages;

    void SetUp() override {
        handler = std::make_shared<config::ConfigHandler>();
        messages.clear();

        // register a callback for tests that need to capture messages
        handler->registerPluginDisplayMessageCallback(
            [this](const std::string& title, const std::string& msg) { messages.push_back(title + ": " + msg); });
    }

    void TearDown() override { messages.clear(); }

    std::string createTempConfigFile(const std::string& contents) {
        namespace fs = std::filesystem;

        fs::path tempDir = fs::temp_directory_path();
        fs::path tempFile = tempDir / fs::path("config_test.txt");

        std::ofstream ofs(tempFile);
        ofs << contents;
        ofs.close();

        return tempFile.string();
    }
};

TEST_F(ConfigHandlerTest, TestObserver) {
    auto mockObserver = std::make_shared<mocks::MockConfigObserver>();
    EXPECT_CALL(*mockObserver, onConfigChange(::testing::_)).Times(1);

    handler->subscribe(mockObserver);
    handler->notify();
}

TEST_F(ConfigHandlerTest, MultipleObservers) {
    auto obs1 = std::make_shared<mocks::MockConfigObserver>();
    auto obs2 = std::make_shared<mocks::MockConfigObserver>();

    EXPECT_CALL(*obs1, onConfigChange(::testing::_)).Times(1);
    EXPECT_CALL(*obs2, onConfigChange(::testing::_)).Times(1);

    handler->subscribe(obs1);
    handler->subscribe(obs2);

    handler->notify();
}

TEST_F(ConfigHandlerTest, ObserverUnsubscribe) {
    auto obs = std::make_shared<mocks::MockConfigObserver>();
    EXPECT_CALL(*obs, onConfigChange(::testing::_)).Times(0);

    handler->subscribe(obs);
    handler->unsubscribe(obs);
    handler->notify();
}

TEST_F(ConfigHandlerTest, ExpiredObserverNotNotified) {
    auto obs = std::make_shared<mocks::MockConfigObserver>();
    EXPECT_CALL(*obs, onConfigChange(::testing::_)).Times(0);

    handler->subscribe(obs);
    obs.reset();  // simulate expiration
    handler->notify();
}

TEST_F(ConfigHandlerTest, MultipleNotifications) {
    auto obs = std::make_shared<mocks::MockConfigObserver>();
    EXPECT_CALL(*obs, onConfigChange(::testing::_)).Times(2);

    handler->subscribe(obs);
    handler->notify();
    handler->notify();
}

TEST_F(ConfigHandlerTest, LoadValidConfig) {
    std::string configText = R"(
        SERVER_url=http://localhost
        UPDATE_RATE_SECONDS=5
        COLOR_lightgreen=0,255,0
        COLOR_blue=0,0,255
    )";
    std::string filename = createTempConfigFile(configText);

    config::ConfigHandler handler2(filename);
    handler2.load(true);

    EXPECT_TRUE(handler2.getConfig().valid);
    EXPECT_TRUE(messages.empty());
}

TEST_F(ConfigHandlerTest, LoadInvalidUpdateRate) {
    std::string configText = R"(
        UPDATE_RATE_SECONDS=9999
    )";
    std::string filename = createTempConfigFile(configText);
    config::ConfigHandler handler2(filename);
    std::vector<std::string> _messages;
    handler2.registerPluginDisplayMessageCallback(
        [&_messages](const std::string& title, const std::string& msg) { _messages.push_back(title + ": " + msg); });

    handler2.load(true);

    ASSERT_FALSE(_messages.empty());
    EXPECT_THAT(_messages[0], testing::HasSubstr("UPDATE_RATE_SECONDS must be between"));
}

TEST_F(ConfigHandlerTest, LoadInvalidColor) {
    std::string configText = R"(COLOR_lightgreen=0,abc,0)";
    std::string filename = createTempConfigFile(configText);
    config::ConfigHandler handler2(filename);
    std::vector<std::string> _messages;
    handler2.registerPluginDisplayMessageCallback(
        [&_messages](const std::string& title, const std::string& msg) { _messages.push_back(title + ": " + msg); });

    handler2.load(true);

    ASSERT_FALSE(_messages.empty());
    EXPECT_THAT(_messages[0], testing::HasSubstr("Failed"));
}