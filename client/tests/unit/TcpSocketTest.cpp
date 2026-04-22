#include "../../srcs/net/TcpSocket.hpp"
#include <gtest/gtest.h>

// TestFixture
class TcpSocketTest : public ::testing::Test {
protected:
    TcpSocket socket_;
};

// State Transition Tests -> No Network I/O
TEST_F(TcpSocketTest, InitialStateIsDisconnected) {
    EXPECT_EQ(socket_.state(), TcpState::Disconnected);
    EXPECT_FALSE(socket_.isConnected());
    EXPECT_FALSE(socket_.isConnecting());
    EXPECT_FALSE(socket_.isOpen());
}

TEST_F(TcpSocketTest, FdReturnsNegativeWhenDisconnected) {
    int fd = socket_.fd();
    EXPECT_LT(fd, 0);
}

TEST_F(TcpSocketTest, ConnectToInvalidHostReturnsFailure) {
    Result res = socket_.connectTo("", 12345);
    EXPECT_FALSE(res.ok());
    EXPECT_EQ(socket_.state(), TcpState::Disconnected);
}

TEST_F(TcpSocketTest, ConnectToInvalidPortZeroReturnsFailure) {
    Result res = socket_.connectTo("127.0.0.1", 0);
    EXPECT_FALSE(res.ok());
    EXPECT_EQ(socket_.state(), TcpState::Disconnected);
}

TEST_F(TcpSocketTest, ConnectToInvalidPortTooHighReturnsFailure) {
    Result res = socket_.connectTo("127.0.0.1", 99999);
    EXPECT_FALSE(res.ok());
    EXPECT_EQ(socket_.state(), TcpState::Disconnected);
}

// Read/Write on Disconnected Socket -> Should Fail
TEST_F(TcpSocketTest, ReadSomeOnDisconnectedReturnsInvalidState) {
    std::vector<std::uint8_t> buffer;
    IoResult res = socket_.readSome(buffer, 100);
    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "readSome called while socket is not connected");
}

TEST_F(TcpSocketTest, WriteSomeOnDisconnectedReturnsInvalidState) {
    std::vector<std::uint8_t> data = {1, 2, 3};
    IoResult res = socket_.writeSome(data, 0);
    EXPECT_EQ(res.status, NetStatus::InvalidState);
    EXPECT_EQ(res.message, "writeSome called while socket is not connected");
}

TEST_F(TcpSocketTest, WriteSomeWithInvalidOffsetReturnsInvalidState) {
    std::vector<std::uint8_t> data = {1, 2, 3};
    IoResult res = socket_.writeSome(data, 10);  // Offset out of range + disconnected
    EXPECT_EQ(res.status, NetStatus::InvalidState);
    // Connection check happens before offset check
    EXPECT_EQ(res.message, "writeSome called while socket is not connected");
}

// Close Operations
TEST_F(TcpSocketTest, CloseOnDisconnectedSocketIsSafe) {
    socket_.close();  // Should not crash
    EXPECT_FALSE(socket_.isConnected());
    EXPECT_FALSE(socket_.isOpen());
}

TEST_F(TcpSocketTest, DoubleCloseIsSafe) {
    socket_.close();
    socket_.close();  // Should not crash
    EXPECT_FALSE(socket_.isConnected());
}

// PollConnect on Disconnected -> Should Fail
TEST_F(TcpSocketTest, PollConnectOnDisconnectedReturnsFail) {
    Result res = socket_.pollConnect(0);
    EXPECT_FALSE(res.ok());
}

// IoResult Structure Tests
TEST_F(TcpSocketTest, IoResultInitializesWithOkStatus) {
    IoResult res{};
    EXPECT_EQ(res.status, NetStatus::Ok);
    EXPECT_EQ(res.bytes, 0u);
    EXPECT_EQ(res.sysErrno, 0);
}

TEST_F(TcpSocketTest, IoResultCanBeModified) {
    IoResult res{};
    res.status = NetStatus::WouldBlock;
    res.bytes = 42;
    res.sysErrno = EAGAIN;
    res.message = "test";

    EXPECT_EQ(res.status, NetStatus::WouldBlock);
    EXPECT_EQ(res.bytes, 42u);
    EXPECT_EQ(res.sysErrno, EAGAIN);
    EXPECT_EQ(res.message, "test");
}

// TcpState Enum Tests
TEST_F(TcpSocketTest, TcpStateEnumValuesAreDistinct) {
    EXPECT_NE(TcpState::Disconnected, TcpState::Connecting);
    EXPECT_NE(TcpState::Connecting, TcpState::Connected);
    EXPECT_NE(TcpState::Connected, TcpState::Closed);
    EXPECT_NE(TcpState::Disconnected, TcpState::Closed);
}

// NetStatus Enum Tests
TEST_F(TcpSocketTest, NetStatusEnumValuesAreDistinct) {
    EXPECT_NE(NetStatus::Ok, NetStatus::WouldBlock);
    EXPECT_NE(NetStatus::WouldBlock, NetStatus::ConnectionClosed);
    EXPECT_NE(NetStatus::ConnectionClosed, NetStatus::InvalidState);
    EXPECT_NE(NetStatus::InvalidState, NetStatus::NetworkError);
}