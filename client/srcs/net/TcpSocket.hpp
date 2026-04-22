#pragma once

#include "../../incs/Result.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class TcpState {
	Disconnected,
	Connecting,
	Connected,
	Closed
};

enum class NetStatus {
	Ok,
	WouldBlock,
	Connecting,
	ConnectionClosed,
	Timeout,
	InvalidState,
	NetworkError
};

struct IoResult {
	NetStatus	status = NetStatus::Ok;
	std::size_t	bytes = 0;
	std::string	message;
	int			sysErrno = 0;
};

class TcpSocket {
	private:
		int			_fd = -1;
		TcpState	_state = TcpState::Disconnected;
		std::string	_host;
		int			_port = -1;
		int			_lastErrno = 0;
		std::string	_lastError;

	private:
		Result	createSocketAndStartConnect(const std::string& host, int port);
		Result	setNonBlocking(int socketFd);
		Result	finalizeConnectState();
		void	setLastError(int err, const std::string& msg);

	public:
		TcpSocket() = default;
		~TcpSocket();

		TcpSocket(const TcpSocket&) = delete;
		TcpSocket& operator=(const TcpSocket&) = delete;

		Result	connectTo(const std::string& host, int port);
		Result	pollConnect(int timeoutMs);
		void	close();

		bool	isConnected() const;
		bool	isConnecting() const;
		bool	isOpen() const;

		int			fd() const;
		TcpState	state() const;

		IoResult	readSome(std::vector<std::uint8_t>& out, std::size_t maxBytes);
		IoResult	writeSome(const std::vector<std::uint8_t>& data, std::size_t offset);

		std::string	lastErrorString() const;
		int			lastErrno() const;
};