#include "../../srcs/net/WebsocketClient.hpp"
#include <gtest/gtest.h>

class WebsocketClientTest : public ::testing::Test {
protected:
    WebsocketClient client_;
};

TEST_F(WebsocketClientTest, InitialStateStartsDisconnected) {
    EXPECT_FALSE(client_.isConnected());
    EXPECT_FALSE(client_.isConnecting());
    EXPECT_FALSE(client_.isOpen());
    EXPECT_EQ(client_.sendQueueSize(), 0u);
}

TEST_F(WebsocketClientTest, CloseOnFreshClientIsSafe) {
    client_.close();
    EXPECT_FALSE(client_.isConnected());
    EXPECT_FALSE(client_.isConnecting());
    EXPECT_FALSE(client_.isOpen());
    EXPECT_EQ(client_.sendQueueSize(), 0u);
}

TEST_F(WebsocketClientTest, InvalidConnectArgumentsAreRejected) {
    Result res = client_.connect("", 8674, false);

    EXPECT_FALSE(res.ok());
    EXPECT_EQ(res.code, ErrorCode::InvalidArgs);
    EXPECT_EQ(res.message, "Invalid host or port");
}

TEST_F(WebsocketClientTest, SendTextWhileDisconnectedReturnsInvalidState) {
    IoResult res = client_.sendText("hello");

    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "WebSocket not connected");
    EXPECT_EQ(client_.sendQueueSize(), 0u);
}

TEST_F(WebsocketClientTest, SendPingWhileDisconnectedReturnsInvalidState) {
    IoResult res = client_.sendPing();

    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "WebSocket not connected");
}

TEST_F(WebsocketClientTest, SendPongWhileDisconnectedReturnsInvalidState) {
    IoResult res = client_.sendPong();

    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "WebSocket not connected");
}