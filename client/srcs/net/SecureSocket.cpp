#include "SecureSocket.hpp"
#include "TlsContext.hpp"

#include "../helpers/Logger.hpp"
#include <openssl/err.h>
#include <iostream>
#include <cstring>

SecureSocket::SecureSocket()
    : _tcp(std::make_unique<TcpSocket>()) {
}

SecureSocket::~SecureSocket() {
    close();
}

Result SecureSocket::connectTo(const std::string& host, int port, bool insecure) {
    if (!_tcp) {
        return Result::failure(ErrorCode::InternalError, "TCP socket not initialized");
    }

    // Initialize TLS context if needed
    TlsContext& ctx = TlsContext::instance();
    if (!ctx.isInitialized()) {
        Result res = ctx.initialize(insecure);
        if (!res.ok()) {
            return res;
        }
    }

    // Start TCP connection
    Result tcp_res = _tcp->connectTo(host, port);
    if (!tcp_res.ok()) {
        return tcp_res;
    }

    // If TCP is immediately connected, start handshake
    if (_tcp->isConnected()) {
        return performHandshake();
    }

    // TCP is connecting, handshake will happen in pollConnect
    return Result::success();
}

Result SecureSocket::pollConnect(int timeoutMs) {
    if (!_tcp) {
        return Result::failure(ErrorCode::InternalError, "TCP socket not initialized");
    }

    // Poll TCP connection first
    if (_tcp->isConnecting()) {
        Result tcp_res = _tcp->pollConnect(timeoutMs);
        if (!tcp_res.ok()) {
            return tcp_res;
        }
    }

    if (!_tcp->isConnected()) {
        return Result::success();
    }

    // TCP is now connected, perform TLS handshake
    if (!_handshake_done) {
        return performHandshake();
    }

    return Result::success();
}

void SecureSocket::close() {
    if (_ssl) {
        SSL_shutdown(_ssl);
        SSL_free(_ssl);
        _ssl = nullptr;
    }

    if (_tcp) {
        _tcp->close();
    }

    _handshake_done = false;
}

bool SecureSocket::isConnected() const {
    return _tcp && _tcp->isConnected() && _handshake_done;
}

bool SecureSocket::isConnecting() const {
    return _tcp && _tcp->isConnecting();
}

bool SecureSocket::isHandshakeDone() const {
    return _handshake_done;
}

bool SecureSocket::isOpen() const {
    return _tcp && _tcp->isOpen();
}

IoResult SecureSocket::tlsRead(std::vector<std::uint8_t>& out, std::size_t maxBytes) {
    IoResult res{};

    if (!isConnected()) {
        res.status = NetStatus::InvalidState;
        res.message = "tlsRead called while not fully connected";
        return res;
    }

    if (maxBytes == 0) {
        res.status = NetStatus::Ok;
        return res;
    }

    std::vector<std::uint8_t> tmp(maxBytes);
    const int n = SSL_read(_ssl, tmp.data(), maxBytes);
    
    // DEBUG
    //std::cerr << "[tlsRead] SSL_read returned: " << n << " (requested: " << maxBytes << ")\n" ;
    //if (n > 0) std::cerr << "[tlsRead] First 20 bytes: ";
    //for (int i = 0; i < std::min(n, 20); i++) std::cerr << (int)tmp[i] << " ";
    //std::cerr << "\n";

    if (n > 0) {
        out.insert(out.end(), tmp.begin(), tmp.begin() + n);
        res.status = NetStatus::Ok;
        res.bytes = static_cast<std::size_t>(n);
        return res;
    }

    if (n == 0) {
        // SSL shutdown by peer
        res.status = NetStatus::ConnectionClosed;
        res.message = "TLS: Peer closed connection";
        close();
        return res;
    }

    // n < 0: error or need more data
    const int ssl_err = SSL_get_error(_ssl, n);

    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
        res.status = NetStatus::WouldBlock;
        res.message = "TLS: Need more data";
        res.sysErrno = ssl_err;
        return res;
    }

    // Real error
    char err_buf[256];
    ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
    _last_error = std::string("SSL_read failed: ") + std::string(err_buf);
    _last_ssl_error = ssl_err;

    res.status = NetStatus::NetworkError;
    res.message = _last_error;
    res.sysErrno = ssl_err;

    Logger::error(_last_error);
    close();
    return res;
}

IoResult SecureSocket::tlsWrite(const std::vector<std::uint8_t>& data, std::size_t offset) {
    IoResult res{};

    if (!isConnected()) {
        res.status = NetStatus::InvalidState;
        res.message = "tlsWrite called while not fully connected";
        return res;
    }

    if (offset >= data.size()) {
        res.status = NetStatus::InvalidState;
        res.message = "tlsWrite offset is out of range";
        return res;
    }

    const std::uint8_t* ptr = data.data() + offset;
    const std::size_t len = data.size() - offset;

    const int n = SSL_write(_ssl, ptr, len);

    if (n > 0) {
        res.status = NetStatus::Ok;
        res.bytes = static_cast<std::size_t>(n);
        return res;
    }

    if (n == 0) {
        res.status = NetStatus::ConnectionClosed;
        res.message = "TLS: Connection closed";
        close();
        return res;
    }

    // n < 0: error or need to retry
    const int ssl_err = SSL_get_error(_ssl, n);

    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
        res.status = NetStatus::WouldBlock;
        res.message = "TLS: Need to retry write";
        res.sysErrno = ssl_err;
        return res;
    }

    // Real error
    char err_buf[256];
    ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
    _last_error = std::string("SSL_write failed: ") + std::string(err_buf);
    _last_ssl_error = ssl_err;

    res.status = NetStatus::NetworkError;
    res.message = _last_error;
    res.sysErrno = ssl_err;

    Logger::error(_last_error);
    close();
    return res;
}

std::string SecureSocket::lastErrorString() const {
    return _last_error;
}

int SecureSocket::lastSslError() const {
    return _last_ssl_error;
}

TcpSocket* SecureSocket::tcp() const {
    return _tcp.get();
}

// Private Implementation
Result SecureSocket::performHandshake() {
    if (!_tcp || !_tcp->isConnected()) {
        return Result::failure(ErrorCode::InternalError, "TCP not connected for handshake");
    }

    // Create SSL object if not already created
    if (!_ssl) {
        TlsContext& ctx = TlsContext::instance();
        _ssl = SSL_new(ctx.getCtx());
        if (!_ssl) {
            const char* err = ERR_reason_error_string(ERR_get_error());
            std::string msg = std::string("SSL_new failed: ") + (err ? err : "unknown error");
            Logger::error(msg);
            return Result::failure(ErrorCode::NetworkError, msg);
        }

        // Set socket file descriptor for SSL
        if (!SSL_set_fd(_ssl, _tcp->fd())) {
            const char* err = ERR_reason_error_string(ERR_get_error());
            std::string msg = std::string("SSL_set_fd failed: ") + (err ? err : "unknown error");
            Logger::error(msg);
            SSL_free(_ssl);
            _ssl = nullptr;
            return Result::failure(ErrorCode::NetworkError, msg);
        }

        // Set to connect state (client mode)
        SSL_set_connect_state(_ssl);
    }

    // Perform handshake
    const int ret = SSL_do_handshake(_ssl);

    if (ret == 1) {
        _handshake_done = true;
        Logger::info("TLS handshake completed successfully");
        return Result::success();
    }

    if (ret == 0) {
        // Clean shutdown by peer (shouldn't happen during handshake)
        setLastError(0, "Peer closed connection during handshake");
        close();
        return Result::failure(ErrorCode::NetworkError, _last_error);
    }

    // ret < 0: error or need to retry
    const int ssl_err = SSL_get_error(_ssl, ret); std::cout << "SSL ret=" << ret << " err=" << ssl_err << std::endl;

    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
        // Handshake in progress, need more data
        return Result::success();  // Keep trying
    }

    // Real error
    char err_buf[256];
    ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
    _last_error = std::string("SSL_do_handshake failed: ") + std::string(err_buf);
    _last_ssl_error = ssl_err;

    Logger::error(_last_error);
    close();
    return Result::failure(ErrorCode::NetworkError, _last_error);
}

void SecureSocket::setLastError(int ssl_err, const std::string& msg) {
    _last_ssl_error = ssl_err;
    _last_error = msg;
}
