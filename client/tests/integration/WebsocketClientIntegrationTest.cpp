#include "../../srcs/net/WebsocketClient.hpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <openssl/sha.h>
#include <string>
#include <thread>
#include <vector>


constexpr const char* kCertPath = "../server/certs/cert.pem";
constexpr const char* kKeyPath = "../server/certs/key.pem";
constexpr const char* kWebSocketGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

std::string base64Encode(const std::vector<std::uint8_t>& data) {
    static constexpr const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    int value = 0;
    int value_bits = -6;

    for (std::uint8_t byte : data) {
        value = (value << 8) + byte;
        value_bits += 8;
        while (value_bits >= 0) {
            result.push_back(base64_chars[(value >> value_bits) & 0x3F]);
            value_bits -= 6;
        }
    }

    if (value_bits > -6) {
        result.push_back(base64_chars[((value << 8) >> (value_bits + 8)) & 0x3F]);
    }

    while (result.size() % 4) {
        result.push_back('=');
    }

    return result;
}

std::string websocketAcceptValue(const std::string& key) {
    std::string challenge = key + kWebSocketGuid;
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(challenge.data()), challenge.size(), hash);
    return base64Encode(std::vector<std::uint8_t>(hash, hash + SHA_DIGEST_LENGTH));
}

std::string extractHeaderValue(const std::string& headers, const std::string& header_name) {
    const std::string prefix = header_name + ":";
    std::size_t start = headers.find(prefix);
    if (start == std::string::npos) {
        return {};
    }

    start += prefix.size();
    while (start < headers.size() && (headers[start] == ' ' || headers[start] == '\t')) {
        ++start;
    }

    std::size_t end = headers.find("\r\n", start);
    if (end == std::string::npos) {
        end = headers.size();
    }

    return headers.substr(start, end - start);
}

int createListeningSocket(int& out_port) {
    int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        return -1;
    }

    int reuse = 1;
    ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0);

    if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        ::close(listener);
        return -1;
    }

    if (::listen(listener, 1) != 0) {
        ::close(listener);
        return -1;
    }

    socklen_t len = sizeof(address);
    if (::getsockname(listener, reinterpret_cast<sockaddr*>(&address), &len) != 0) {
        ::close(listener);
        return -1;
    }

    out_port = ntohs(address.sin_port);
    return listener;
}

struct ServerState {
    std::string request;
    std::string received_text;
    std::string error;
    bool handshake_ok = false;
};

class ScopedFd {
public:
    explicit ScopedFd(int fd = -1) : _fd(fd) {}
    ~ScopedFd() { reset(); }

    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;

    int get() const { return _fd; }

    int release() {
        int fd = _fd;
        _fd = -1;
        return fd;
    }

    void reset(int fd = -1) {
        if (_fd >= 0) {
            ::close(_fd);
        }
        _fd = fd;
    }

private:
    int _fd;
};

class LoopbackWebSocketServer {
public:
    explicit LoopbackWebSocketServer(int listener_fd)
        : _listener_fd(listener_fd), _thread(&LoopbackWebSocketServer::run, this) {}

    ~LoopbackWebSocketServer() {
        stop();
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    void stop() {
        if (_listener_fd >= 0) {
            ::shutdown(_listener_fd, SHUT_RDWR);
            ::close(_listener_fd);
            _listener_fd = -1;
        }
    }

    bool waitUntilReady(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(_mutex);
        return _cv.wait_for(lock, timeout, [&] { return _ready; });
    }

    bool waitUntilDone(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(_mutex);
        return _cv.wait_for(lock, timeout, [&] { return _done; });
    }

    ServerState state() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _state;
    }

private:
    static bool readHttpHeaders(SSL* ssl, std::string& headers, std::string& error_message) {
        std::vector<char> buffer(1024);

        for (int attempt = 0; attempt < 200; ++attempt) {
            int read_count = SSL_read(ssl, buffer.data(), static_cast<int>(buffer.size()));
            if (read_count > 0) {
                headers.append(buffer.data(), read_count);
                if (headers.find("\r\n\r\n") != std::string::npos) {
                    return true;
                }
                continue;
            }

            int ssl_error = SSL_get_error(ssl, read_count);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            error_message = "Failed to read WebSocket upgrade request";
            return false;
        }

        error_message = "Timed out waiting for WebSocket upgrade request";
        return false;
    }

    static bool readWebSocketFrame(SSL* ssl, WebSocketFrame& frame_out, std::string& error_message) {
        std::vector<std::uint8_t> raw;
        raw.reserve(128);
        std::size_t offset = 0;

        for (int attempt = 0; attempt < 200; ++attempt) {
            std::uint8_t buffer[256];
            int read_count = SSL_read(ssl, buffer, sizeof(buffer));
            if (read_count > 0) {
                raw.insert(raw.end(), buffer, buffer + read_count);
                Result decode_result = FrameCodec::decodeFrame(raw, offset, frame_out);
                if (decode_result.ok()) {
                    return true;
                }

                if (decode_result.message.find("Not enough data") != std::string::npos ||
                    decode_result.message.find("Incomplete") != std::string::npos) {
                    continue;
                }

                error_message = decode_result.message;
                return false;
            }

            int ssl_error = SSL_get_error(ssl, read_count);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            error_message = "Failed to read WebSocket frame";
            return false;
        }

        error_message = "Timed out waiting for WebSocket frame";
        return false;
    }

    void run() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _ready = true;
        }
        _cv.notify_all();

        ScopedFd client_fd;

        pollfd listener_poll{};
        listener_poll.fd = _listener_fd;
        listener_poll.events = POLLIN;

        if (poll(&listener_poll, 1, 5000) <= 0) {
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "Timed out waiting for client connect";
            _done = true;
            _cv.notify_all();
            return;
        }

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        client_fd.reset(::accept(_listener_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len));
        if (client_fd.get() < 0) {
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "accept failed";
            _done = true;
            _cv.notify_all();
            return;
        }

        timeval timeout{};
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        ::setsockopt(client_fd.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        ::setsockopt(client_fd.get(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
        if (!ctx) {
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "SSL_CTX_new failed";
            _done = true;
            _cv.notify_all();
            return;
        }

        SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
        if (SSL_CTX_use_certificate_file(ctx, kCertPath, SSL_FILETYPE_PEM) != 1 ||
            SSL_CTX_use_PrivateKey_file(ctx, kKeyPath, SSL_FILETYPE_PEM) != 1) {
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "Failed to load TLS certificate material";
            _done = true;
            _cv.notify_all();
            return;
        }

        SSL* ssl = SSL_new(ctx);
        if (!ssl) {
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "SSL_new failed";
            _done = true;
            _cv.notify_all();
            return;
        }

        SSL_set_fd(ssl, client_fd.get());
        if (SSL_accept(ssl) != 1) {
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "TLS handshake failed";
            _done = true;
            _cv.notify_all();
            return;
        }

        std::string request;
        std::string read_error;
        if (!readHttpHeaders(ssl, request, read_error)) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = read_error;
            _done = true;
            _cv.notify_all();
            return;
        }

        const std::size_t header_end = request.find("\r\n\r\n");
        request.resize(header_end + 4);
        const std::string ws_key = extractHeaderValue(request, "Sec-WebSocket-Key");
        if (ws_key.empty()) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "Missing Sec-WebSocket-Key header";
            _done = true;
            _cv.notify_all();
            return;
        }

        const std::string accept_value = websocketAcceptValue(ws_key);
        std::ostringstream response;
        response << "HTTP/1.1 101 Switching Protocols\r\n"
                << "Upgrade: websocket\r\n"
                << "Connection: Upgrade\r\n"
                << "Sec-WebSocket-Accept: " << accept_value << "\r\n"
                << "\r\n";
        const std::string response_text = response.str();
        if (SSL_write(ssl, response_text.data(), static_cast<int>(response_text.size())) <= 0) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "Failed to send handshake response";
            _done = true;
            _cv.notify_all();
            return;
        }

        const std::string greeting = "welcome integration";
        std::vector<std::uint8_t> greeting_frame;
        greeting_frame.push_back(0x81);
        greeting_frame.push_back(static_cast<std::uint8_t>(greeting.size()));
        greeting_frame.insert(greeting_frame.end(), greeting.begin(), greeting.end());
        if (SSL_write(ssl, greeting_frame.data(), static_cast<int>(greeting_frame.size())) <= 0) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = "Failed to send greeting frame";
            _done = true;
            _cv.notify_all();
            return;
        }

        WebSocketFrame received_frame;
        std::string frame_error;
        if (!readWebSocketFrame(ssl, received_frame, frame_error)) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            std::lock_guard<std::mutex> lock(_mutex);
            _state.error = frame_error;
            _done = true;
            _cv.notify_all();
            return;
        }

        std::string received_text(received_frame.payload.begin(), received_frame.payload.end());

        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _state.request = request;
            _state.received_text = received_text;
            _state.handshake_ok = true;
            _done = true;
        }
        _cv.notify_all();
    }

    int _listener_fd = -1;
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    bool _ready = false;
    bool _done = false;
    ServerState _state;
    std::thread _thread;
};

bool waitForClientConnected(WebsocketClient& client, int64_t& now_ms) {
    for (int attempt = 0; attempt < 400; ++attempt) {
        Result tick_result = client.tick(now_ms);
        if (client.isConnected()) {
            return true;
        }

        if (!tick_result.ok() && client.isOpen() == false) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        now_ms += 5;
    }

    return client.isConnected();
}

bool waitForServerGreeting(WebsocketClient& client, WebSocketFrame& out_frame, int64_t& now_ms) {
    for (int attempt = 0; attempt < 200; ++attempt) {
        client.tick(now_ms);

        IoResult recv_result = client.recvFrame(out_frame);
        if (recv_result.status == NetStatus::Ok) {
            return true;
        }

        if (recv_result.status != NetStatus::WouldBlock) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        now_ms += 5;
    }

    return false;
}

TEST(WebsocketClientIntegrationTest, ConnectsAndExchangesFramesOverTlsWebSocket) {
    int port = 0;
    int listener = createListeningSocket(port);
    ASSERT_GE(listener, 0);

    LoopbackWebSocketServer server(listener);
    ASSERT_TRUE(server.waitUntilReady(std::chrono::seconds(2)));

    WebsocketClient client;
    Result connect_result = client.connect("127.0.0.1", port, true);
    ASSERT_TRUE(connect_result.ok()) << connect_result.message;

    int64_t now_ms = 0;
    ASSERT_TRUE(waitForClientConnected(client, now_ms));

    WebSocketFrame greeting_frame;
    ASSERT_TRUE(waitForServerGreeting(client, greeting_frame, now_ms));
    EXPECT_EQ(greeting_frame.opcode, WebSocketOpcode::Text);
    EXPECT_EQ(std::string(greeting_frame.payload.begin(), greeting_frame.payload.end()), "welcome integration");

    IoResult send_result = client.sendText("hello integration");
    ASSERT_EQ(send_result.status, NetStatus::Ok) << send_result.message;

    for (int attempt = 0; attempt < 40; ++attempt) {
        client.tick(now_ms);
        if (server.waitUntilDone(std::chrono::milliseconds(0))) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        now_ms += 5;
    }

    ASSERT_TRUE(server.waitUntilDone(std::chrono::seconds(3))) << server.state().error;

    ServerState server_state = server.state();
    ASSERT_TRUE(server_state.error.empty()) << server_state.error;
    EXPECT_TRUE(server_state.handshake_ok);
    EXPECT_NE(server_state.request.find("GET / HTTP/1.1"), std::string::npos);
    EXPECT_NE(server_state.request.find("Upgrade: websocket"), std::string::npos);
    EXPECT_NE(server_state.request.find("Sec-WebSocket-Version: 13"), std::string::npos);
    EXPECT_EQ(server_state.received_text, "hello integration");

    client.close();
}