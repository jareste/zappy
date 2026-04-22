#include "FrameCodec.hpp"

#include "../helpers/Logger.hpp"
#include <cstring>
#include <random>
#include <stdexcept>

// Encode Frame (Client -> Server, with masking)
Result FrameCodec::encodeFrame(const WebSocketFrame& frame, std::vector<std::uint8_t>& out) {
    std::vector<std::uint8_t> header;

    // Byte 0: FIN + RSV + Opcode
    uint8_t byte0 = static_cast<uint8_t>(frame.opcode);
    if (frame.fin) {
        byte0 |= 0x80;  // Set FIN bit
    }
    header.push_back(byte0);

    // Byte 1: MASK + Payload Length
    uint64_t payload_len = frame.payload.size();
    uint8_t mask_bit = 0x80;  // Client frames MUST be masked

    if (payload_len < 126) {
        header.push_back(mask_bit | static_cast<uint8_t>(payload_len));
    } else if (payload_len < 65536) {
        header.push_back(mask_bit | 126);
        header.push_back((payload_len >> 8) & 0xFF);
        header.push_back(payload_len & 0xFF);
    } else {
        header.push_back(mask_bit | 127);
        for (int i = 7; i >= 0; --i) {
            header.push_back((payload_len >> (i * 8)) & 0xFF);
        }
    }

    // Generate masking key
    auto masking_key = generateMaskingKey();
    header.insert(header.end(), masking_key.begin(), masking_key.end());

    // Mask payload
    std::vector<std::uint8_t> masked_payload = frame.payload;
    applyMask(masked_payload, masking_key.data());

    // Assemble: header + masked_payload
    out.insert(out.end(), header.begin(), header.end());
    out.insert(out.end(), masked_payload.begin(), masked_payload.end());

    return Result::success();
}

// Decode Frame (Server -> Client, no masking expected)
Result FrameCodec::decodeFrame(const std::vector<std::uint8_t>& data, std::size_t& offset, WebSocketFrame& out) {
    if (offset >= data.size()) {
        return Result::failure(ErrorCode::ProtocolError, "Not enough data to decode frame header");
    }

    size_t pos = offset;

    // Byte 0: FIN + RSV + Opcode
    if (pos >= data.size()) {
        return Result::failure(ErrorCode::ProtocolError, "Incomplete frame header");
    }

    uint8_t byte0 = data[pos++];
    out.fin = (byte0 & 0x80) != 0;
    uint8_t opcode_val = byte0 & 0x0F;
    out.opcode = static_cast<WebSocketOpcode>(opcode_val);

    // Byte 1: MASK + Payload Length
    if (pos >= data.size()) {
        return Result::failure(ErrorCode::ProtocolError, "Incomplete frame header");
    }

    uint8_t byte1 = data[pos++];
    bool masked = (byte1 & 0x80) != 0;
    uint64_t payload_len = byte1 & 0x7F;

    // Extended payload length
    if (payload_len == 126) {
        if (pos + 2 > data.size()) {
            return Result::failure(ErrorCode::ProtocolError, "Incomplete extended payload length");
        }
        payload_len = (static_cast<uint64_t>(data[pos]) << 8) | data[pos + 1];
        pos += 2;
    } else if (payload_len == 127) {
        if (pos + 8 > data.size()) {
            return Result::failure(ErrorCode::ProtocolError, "Incomplete extended payload length");
        }
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | data[pos++];
        }
    }

    // Masking key (4 bytes if masked)
    std::vector<uint8_t> masking_key;
    if (masked) {
        if (pos + 4 > data.size()) {
            return Result::failure(ErrorCode::ProtocolError, "Incomplete masking key");
        }
        masking_key.insert(masking_key.end(), data.begin() + pos, data.begin() + pos + 4);
        pos += 4;
    }

    // Payload
    if (pos + payload_len > data.size()) {
        return Result::failure(ErrorCode::ProtocolError, "Incomplete payload");
    }

    out.payload.insert(out.payload.end(), data.begin() + pos, data.begin() + pos + payload_len);
    pos += payload_len;

    // Unmask if needed
    if (masked && !out.payload.empty()) {
        applyMask(out.payload, masking_key.data());
    }

    offset = pos;
    return Result::success();
}

// Frame Helpers
WebSocketFrame FrameCodec::createTextFrame(const std::string& text) {
    WebSocketFrame frame;
    frame.opcode = WebSocketOpcode::Text;
    frame.fin = true;
    frame.payload.insert(frame.payload.end(), text.begin(), text.end());
    return frame;
}

WebSocketFrame FrameCodec::createPingFrame() {
    WebSocketFrame frame;
    frame.opcode = WebSocketOpcode::Ping;
    frame.fin = true;
    return frame;
}

WebSocketFrame FrameCodec::createPongFrame() {
    WebSocketFrame frame;
    frame.opcode = WebSocketOpcode::Pong;
    frame.fin = true;
    return frame;
}

WebSocketFrame FrameCodec::createCloseFrame(uint16_t code, const std::string& reason) {
    WebSocketFrame frame;
    frame.opcode = WebSocketOpcode::Close;
    frame.fin = true;

    // Close frame payload: 2-byte status code + reason
    frame.payload.push_back((code >> 8) & 0xFF);
    frame.payload.push_back(code & 0xFF);
    frame.payload.insert(frame.payload.end(), reason.begin(), reason.end());

    return frame;
}

// Private Helpers
std::vector<std::uint8_t> FrameCodec::generateMaskingKey() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::vector<std::uint8_t> key(MASKING_KEY_SIZE);
    for (auto& byte : key) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    return key;
}

void FrameCodec::applyMask(std::vector<std::uint8_t>& data, const uint8_t* mask) {
    if (!mask || data.empty()) {
        return;
    }

    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= mask[i % MASKING_KEY_SIZE];
    }
}

