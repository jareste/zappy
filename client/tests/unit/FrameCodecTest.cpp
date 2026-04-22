#include "../../srcs/net/FrameCodec.hpp"
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
std::string toString(const std::vector<std::uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

}

TEST(FrameCodecTest, CreateTextFrameStoresTextPayload) {
    const std::string text = "hello zappy";
    WebSocketFrame frame = FrameCodec::createTextFrame(text);

    EXPECT_EQ(frame.opcode, WebSocketOpcode::Text);
    EXPECT_TRUE(frame.fin);
    EXPECT_EQ(toString(frame.payload), text);
}

TEST(FrameCodecTest, CreateControlFramesUseExpectedOpcodes) {
    WebSocketFrame ping = FrameCodec::createPingFrame();
    WebSocketFrame pong = FrameCodec::createPongFrame();
    WebSocketFrame close = FrameCodec::createCloseFrame(1001, "going away");

    EXPECT_EQ(ping.opcode, WebSocketOpcode::Ping);
    EXPECT_EQ(pong.opcode, WebSocketOpcode::Pong);
    EXPECT_EQ(close.opcode, WebSocketOpcode::Close);
    EXPECT_TRUE(ping.fin);
    EXPECT_TRUE(pong.fin);
    EXPECT_TRUE(close.fin);
    EXPECT_EQ(close.payload.size(), 2u + std::string("going away").size());
    EXPECT_EQ(close.payload[0], 0x03);
    EXPECT_EQ(close.payload[1], 0xE9);
}

TEST(FrameCodecTest, EncodeFrameMasksClientPayloadAndRoundTrips) {
    WebSocketFrame original = FrameCodec::createTextFrame("transport layer");
    std::vector<std::uint8_t> encoded;

    Result encode_res = FrameCodec::encodeFrame(original, encoded);

    ASSERT_TRUE(encode_res.ok());
    ASSERT_GE(encoded.size(), 6u);
    EXPECT_EQ(encoded[0], 0x81);
    EXPECT_TRUE((encoded[1] & 0x80) != 0);
    EXPECT_EQ(static_cast<std::size_t>(encoded[1] & 0x7F), original.payload.size());

    WebSocketFrame decoded;
    std::size_t offset = 0;
    Result decode_res = FrameCodec::decodeFrame(encoded, offset, decoded);

    ASSERT_TRUE(decode_res.ok());
    EXPECT_EQ(offset, encoded.size());
    EXPECT_EQ(decoded.opcode, WebSocketOpcode::Text);
    EXPECT_TRUE(decoded.fin);
    EXPECT_EQ(decoded.payload, original.payload);
}

TEST(FrameCodecTest, EncodeFrameUsesExtendedPayloadLengthForMediumFrames) {
    WebSocketFrame frame;
    frame.opcode = WebSocketOpcode::Binary;
    frame.fin = true;
    frame.payload = std::vector<std::uint8_t>(126, 0x42);

    std::vector<std::uint8_t> encoded;
    Result encode_res = FrameCodec::encodeFrame(frame, encoded);

    ASSERT_TRUE(encode_res.ok());
    ASSERT_GE(encoded.size(), 4u + 4u + frame.payload.size());
    EXPECT_EQ(encoded[0], 0x82);
    EXPECT_EQ(encoded[1] & 0x7F, 126);
    EXPECT_EQ(encoded[2], 0x00);
    EXPECT_EQ(encoded[3], 126);

    WebSocketFrame decoded;
    std::size_t offset = 0;
    Result decode_res = FrameCodec::decodeFrame(encoded, offset, decoded);

    ASSERT_TRUE(decode_res.ok());
    EXPECT_EQ(decoded.opcode, WebSocketOpcode::Binary);
    EXPECT_EQ(decoded.payload, frame.payload);
}

TEST(FrameCodecTest, DecodeFrameRejectsIncompletePayload) {
    const std::vector<std::uint8_t> data = {
        0x81,
        0x03,
        'a',
        'b'
    };

    WebSocketFrame decoded;
    std::size_t offset = 0;
    Result decode_res = FrameCodec::decodeFrame(data, offset, decoded);

    EXPECT_FALSE(decode_res.ok());
    EXPECT_EQ(decode_res.code, ErrorCode::ProtocolError);
    EXPECT_EQ(decode_res.message, "Incomplete payload");
    EXPECT_EQ(offset, 0u);
}

TEST(FrameCodecTest, DecodeFrameRejectsEmptyInput) {
    const std::vector<std::uint8_t> data;
    WebSocketFrame decoded;
    std::size_t offset = 0;

    Result decode_res = FrameCodec::decodeFrame(data, offset, decoded);

    EXPECT_FALSE(decode_res.ok());
    EXPECT_EQ(decode_res.code, ErrorCode::ProtocolError);
    EXPECT_EQ(decode_res.message, "Not enough data to decode frame header");
}