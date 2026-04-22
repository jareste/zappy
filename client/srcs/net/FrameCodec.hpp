#pragma once

#include "../../incs/Result.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class WebSocketOpcode : uint8_t {
    Continuation = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA
};

struct WebSocketFrame {
    WebSocketOpcode opcode = WebSocketOpcode::Text;
    bool fin = true;  // Final frame
    std::vector<std::uint8_t> payload;
};

class FrameCodec {
    public:
        // Encode a frame to bytes (client frames are masked)
        static Result encodeFrame(const WebSocketFrame& frame, std::vector<std::uint8_t>& out);

        // Decode bytes to a frame (returns WantMore if incomplete)
        static Result decodeFrame(const std::vector<std::uint8_t>& data, std::size_t& offset, WebSocketFrame& out);

        // Helper: create text frame
        static WebSocketFrame createTextFrame(const std::string& text);

        // Helper: create ping frame
        static WebSocketFrame createPingFrame();

        // Helper: create pong frame
        static WebSocketFrame createPongFrame();

        // Helper: create close frame
        static WebSocketFrame createCloseFrame(uint16_t code = 1000, const std::string& reason = "");

    private:
        static constexpr uint32_t MASKING_KEY_SIZE = 4;

        // Generate random masking key
        static std::vector<std::uint8_t> generateMaskingKey();

        // Apply/remove XOR masking
        static void applyMask(std::vector<std::uint8_t>& data, const std::uint8_t* mask);
};
