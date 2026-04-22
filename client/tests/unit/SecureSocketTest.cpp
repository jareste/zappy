#include "../../srcs/net/SecureSocket.hpp"
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

class SecureSocketTest : public ::testing::Test {
protected:
    SecureSocket socket_;
};

TEST_F(SecureSocketTest, InitialStateStartsClosedAndUnhandshaked) {
    EXPECT_FALSE(socket_.isConnected());
    EXPECT_FALSE(socket_.isConnecting());
    EXPECT_FALSE(socket_.isHandshakeDone());
    EXPECT_FALSE(socket_.isOpen());
    EXPECT_NE(socket_.tcp(), nullptr);
}

TEST_F(SecureSocketTest, CloseOnFreshSocketIsSafe) {
    socket_.close();
    EXPECT_FALSE(socket_.isConnected());
    EXPECT_FALSE(socket_.isConnecting());
    EXPECT_FALSE(socket_.isHandshakeDone());
    EXPECT_FALSE(socket_.isOpen());
}

TEST_F(SecureSocketTest, TlsReadOnDisconnectedSocketReturnsInvalidState) {
    std::vector<std::uint8_t> buffer;
    IoResult res = socket_.tlsRead(buffer, 32);

    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "tlsRead called while not fully connected");
    EXPECT_TRUE(buffer.empty());
}

TEST_F(SecureSocketTest, TlsWriteOnDisconnectedSocketReturnsInvalidState) {
    const std::vector<std::uint8_t> data = {0x01, 0x02, 0x03};

    IoResult res = socket_.tlsWrite(data, 0);

    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "tlsWrite called while not fully connected");
}

TEST_F(SecureSocketTest, TlsWriteWithOutOfRangeOffsetReturnsInvalidState) {
    const std::vector<std::uint8_t> data = {0x01, 0x02, 0x03};

    IoResult res = socket_.tlsWrite(data, data.size());

    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "tlsWrite called while not fully connected");
}