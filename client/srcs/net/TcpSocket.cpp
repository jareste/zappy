#include "TcpSocket.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>  // FIXED: Added for TCP_NODELAY
#include <unistd.h>
#include <iostream>

namespace {
	bool isWouldBlockErrno(int err) {
		return err == EAGAIN || err == EWOULDBLOCK || err == EINTR;
	}
}

TcpSocket::~TcpSocket() {
	close();
}

Result TcpSocket::connectTo(const std::string& host, int port) {
	if (host.empty() || port <= 0 || port > 65535) {
		return Result::failure(ErrorCode::InvalidArgs, "Invalid host or port");
	}

	if (isOpen()) {
		close();
	}

	_host = host;
	_port = port;
	_lastErrno = 0;
	_lastError.clear();

	return createSocketAndStartConnect(host, port);
}

Result TcpSocket::pollConnect(int timeoutMs) {
	if (_state == TcpState::Connected) {
		return Result::success();
	}

	if (_state != TcpState::Connecting || _fd < 0) {
		return Result::failure(ErrorCode::InternalError, "Socket is not in connecting state");
	}

	if (timeoutMs < 0) {
		timeoutMs = 0;
	}

	pollfd pfd{};
	pfd.fd = _fd;
	pfd.events = POLLOUT | POLLERR | POLLHUP;

	const int rc = ::poll(&pfd, 1, timeoutMs); std::cout << "poll rc=" << rc << " revents=" << pfd.revents << std::endl;
	if (rc == 0) {
		if (timeoutMs == 0) {
			return Result::success();
		}
		return Result::failure(ErrorCode::NetworkError, "Connect timeout");
	}
	if (rc < 0) {
		const int err = errno;
		setLastError(err, std::string("poll() failed: ") + std::strerror(err));
		return Result::failure(ErrorCode::NetworkError, _lastError);
	}

	return finalizeConnectState();
}

void TcpSocket::close() {
	if (_fd >= 0) {
		::shutdown(_fd, SHUT_RDWR);
		::close(_fd);
		_fd = -1;
	}

	if (_state != TcpState::Disconnected) {
		_state = TcpState::Closed;
	}
}

bool TcpSocket::isConnected() const {
	return _state == TcpState::Connected && _fd >= 0;
}

bool TcpSocket::isConnecting() const {
	return _state == TcpState::Connecting && _fd >= 0;
}

bool TcpSocket::isOpen() const {
	return _fd >= 0;
}

int TcpSocket::fd() const {
	return _fd;
}

TcpState TcpSocket::state() const {
	return _state;
}

IoResult TcpSocket::readSome(std::vector<std::uint8_t>& out, std::size_t maxBytes) {
	IoResult res{};

	if (!isConnected()) {
		res.status = NetStatus::InvalidState;
		res.message = "readSome called while socket is not connected";
		return res;
	}

	if (maxBytes == 0) {
		res.status = NetStatus::Ok;
		return res;
	}

	std::vector<std::uint8_t> tmp(maxBytes);
	const ssize_t n = ::recv(_fd, tmp.data(), maxBytes, 0);

	if (n > 0) {
		out.insert(out.end(), tmp.begin(), tmp.begin() + n);
		res.status = NetStatus::Ok;
		res.bytes = static_cast<std::size_t>(n);
		return res;
	}

	if (n == 0) {
		res.status = NetStatus::ConnectionClosed;
		res.message = "Peer closed connection";
		close();
		return res;
	}

	const int err = errno;
	res.sysErrno = err;

	if (isWouldBlockErrno(err)) {
		res.status = NetStatus::WouldBlock;
		res.message = std::strerror(err);
		return res;
	}

	setLastError(err, std::string("recv() failed: ") + std::strerror(err));
	res.status = NetStatus::NetworkError;
	res.message = _lastError;
	close();
	return res;
}

IoResult TcpSocket::writeSome(const std::vector<std::uint8_t>& data, std::size_t offset) {
	IoResult res{};

	if (!isConnected()) {
		res.status = NetStatus::InvalidState;
		res.message = "writeSome called while socket is not connected";
		return res;
	}

	if (offset >= data.size()) {
		res.status = NetStatus::InvalidState;
		res.message = "writeSome offset is out of range";
		return res;
	}

	const std::uint8_t* ptr = data.data() + offset;
	const std::size_t len = data.size() - offset;

	const ssize_t n = ::send(_fd, ptr, len, MSG_NOSIGNAL);

	if (n > 0) {
		res.status = NetStatus::Ok;
		res.bytes = static_cast<std::size_t>(n);
		return res;
	}

	if (n == 0) {
		res.status = NetStatus::WouldBlock;
		return res;
	}

	const int err = errno;
	res.sysErrno = err;

	if (isWouldBlockErrno(err)) {
		res.status = NetStatus::WouldBlock;
		res.message = std::strerror(err);
		return res;
	}

	if (err == EPIPE || err == ECONNRESET || err == ENOTCONN) {
		res.status = NetStatus::ConnectionClosed;
		res.message = std::strerror(err);
		close();
		return res;
	}

	setLastError(err, std::string("send() failed: ") + std::strerror(err));
	res.status = NetStatus::NetworkError;
	res.message = _lastError;
	close();
	return res;
}

std::string TcpSocket::lastErrorString() const {
	return _lastError;
}

int TcpSocket::lastErrno() const {
	return _lastErrno;
}

Result TcpSocket::createSocketAndStartConnect(const std::string& host, int port) {
	addrinfo hints{};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addrinfo* head = nullptr;
	const std::string portStr = std::to_string(port);

	const int gai = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &head);
	if (gai != 0) {
		setLastError(0, std::string("getaddrinfo failed: ") + ::gai_strerror(gai));
		return Result::failure(ErrorCode::NetworkError, _lastError);
	}

	Result finalRes = Result::failure(ErrorCode::NetworkError, "No valid address for connection");

	for (addrinfo* ai = head; ai != nullptr; ai = ai->ai_next) {
		const int fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) {
			setLastError(errno, std::string("socket() failed: ") + std::strerror(errno));
			continue;
		}

		// FIXED: Set TCP_NODELAY for lower latency
		int flag = 1;
		::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

		const Result nbRes = setNonBlocking(fd);
		if (!nbRes.ok()) {
			::close(fd);
			continue;
		}

		const int rc = ::connect(fd, ai->ai_addr, ai->ai_addrlen);
		if (rc == 0) {
			_fd = fd;
			_state = TcpState::Connected;
			_lastErrno = 0;
			_lastError.clear();
			finalRes = Result::success();
			break;
		}

		const int err = errno;
		if (err == EINPROGRESS || err == EWOULDBLOCK) {
			_fd = fd;
			_state = TcpState::Connecting;
			_lastErrno = 0;
			_lastError.clear();
			finalRes = Result::success();
			break;
		}

		setLastError(err, std::string("connect() failed: ") + std::strerror(err));
		::close(fd);
		finalRes = Result::failure(ErrorCode::NetworkError, _lastError);
	}

	::freeaddrinfo(head);
	return finalRes;
}

Result TcpSocket::setNonBlocking(int socketFd) {
	const int flags = ::fcntl(socketFd, F_GETFL, 0);
	if (flags < 0) {
		setLastError(errno, std::string("fcntl(F_GETFL) failed: ") + std::strerror(errno));
		return Result::failure(ErrorCode::IoError, _lastError);
	}

	if (::fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) < 0) {
		setLastError(errno, std::string("fcntl(F_SETFL) failed: ") + std::strerror(errno));
		return Result::failure(ErrorCode::IoError, _lastError);
	}

	return Result::success();
}

Result TcpSocket::finalizeConnectState() {
	if (_fd < 0 || _state != TcpState::Connecting) {
		return Result::failure(ErrorCode::InternalError, "Socket is not in connecting state");
	}

	int soErr = 0;
	socklen_t len = sizeof(soErr);

	if (::getsockopt(_fd, SOL_SOCKET, SO_ERROR, &soErr, &len) < 0) {
		setLastError(errno, std::string("getsockopt(SO_ERROR) failed: ") + std::strerror(errno));
		close();
		return Result::failure(ErrorCode::NetworkError, _lastError);
	}

	if (soErr != 0) {
		setLastError(soErr, std::string("connect finalize failed: ") + std::strerror(soErr));
		close();
		return Result::failure(ErrorCode::NetworkError, _lastError);
	}

	_state = TcpState::Connected;
	return Result::success();
}

void TcpSocket::setLastError(int err, const std::string& msg) {
	_lastErrno = err;
	_lastError = msg;
}