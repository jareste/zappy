#pragma once

#include "TcpSocket.hpp"
#include <openssl/ssl.h>
#include <memory>

class SecureSocket {
    private:
        std::unique_ptr<TcpSocket> _tcp;
        SSL* _ssl = nullptr;
        bool _handshake_done = false;
        int _last_ssl_error = 0;
        std::string _last_error;

        Result performHandshake();
        IoResult handleSslError(int ssl_ret, const std::string& op_name);
        void setLastError(int ssl_err, const std::string& msg);

    public:
        SecureSocket();
        ~SecureSocket();

        SecureSocket(const SecureSocket&) = delete;
        SecureSocket& operator=(const SecureSocket&) = delete;

        // Lifecycle
        Result connectTo(const std::string& host, int port, bool insecure = false);
        Result pollConnect(int timeoutMs);
        void close();

        // Status
        bool isConnected() const;
        bool isConnecting() const;
        bool isHandshakeDone() const;
        bool isOpen() const;

        // I/O with encryption
        IoResult tlsRead(std::vector<std::uint8_t>& out, std::size_t maxBytes);
        IoResult tlsWrite(const std::vector<std::uint8_t>& data, std::size_t offset);

        // Error info
        std::string lastErrorString() const;
        int lastSslError() const;

        // Underlying TCP access (for direct operations if needed)
        TcpSocket* tcp() const;
};