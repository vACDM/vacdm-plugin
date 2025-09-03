#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "config/ConfigHandler.h"
#include "config/IConfigHandler.h"
#include "tests/mocks/ConfigObserverMock.h"

class ConfigHandlerFixture : public ::testing::Test {
   protected:
    std::shared_ptr<interfaces::IConfigHandler> handler;
    void SetUp() override { handler = std::make_shared<config::ConfigHandler>(); }

    void TearDown() override {}
};

TEST_F(ConfigHandlerFixture, TestObserver) {
    auto mockObserver = std::make_shared<mocks::MockConfigObserver>();
    EXPECT_CALL(*mockObserver, onConfigChange(::testing::_)).Times(1);

    handler->subscribe(mockObserver);

    handler->notify();
}

TEST_F(ConfigHandlerFixture, MultipleObservers) {
    auto obs1 = std::make_shared<mocks::MockConfigObserver>();
    auto obs2 = std::make_shared<mocks::MockConfigObserver>();

    EXPECT_CALL(*obs1, onConfigChange(::testing::_)).Times(1);
    EXPECT_CALL(*obs2, onConfigChange(::testing::_)).Times(1);

    handler->subscribe(obs1);
    handler->subscribe(obs2);

    handler->notify();
}

TEST_F(ConfigHandlerFixture, ObserverUnsubscribe) {
    auto obs = std::make_shared<mocks::MockConfigObserver>();

    EXPECT_CALL(*obs, onConfigChange(::testing::_)).Times(0);  // should not be called

    handler->subscribe(obs);
    handler->unsubscribe(obs);  // remove observer

    handler->notify();
}

TEST_F(ConfigHandlerFixture, ExpiredObserverNotNotified) {
    auto obs = std::make_shared<mocks::MockConfigObserver>();

    EXPECT_CALL(*obs, onConfigChange(::testing::_)).Times(0);

    handler->subscribe(obs);

    // Simulate observer going out of scope
    obs.reset();  // weak_ptr in handler should expire

    handler->notify();
}

TEST_F(ConfigHandlerFixture, MultipleNotifications) {
    auto obs = std::make_shared<mocks::MockConfigObserver>();

    EXPECT_CALL(*obs, onConfigChange(::testing::_)).Times(2);

    handler->subscribe(obs);

    handler->notify();
    handler->notify();
}