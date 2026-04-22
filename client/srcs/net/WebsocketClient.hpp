#pragma once

#include "SecureSocket.hpp"
#include "FrameCodec.hpp"
#include "../../incs/Result.hpp"
#include <deque>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

enum class WsState {
	Disconnected,
	Connecting,
	Handshaking,
	Connected,
	Closed
};

class WebsocketClient {
	private:
		std::unique_ptr<SecureSocket> _secure_socket;
		WsState _state = WsState::Disconnected;
		std::string _host;
		int _port = -1;
		bool _insecure = false;

		// Buffers
		std::vector<std::uint8_t> _read_buffer;
		std::deque<std::vector<std::uint8_t>> _send_queue;

		// Frame state
		std::size_t _frame_parse_offset = 0;

		// Ping/pong
		static constexpr int PING_INTERVAL_SECONDS = 45;
		int64_t _last_ping_time_ms = 0;

		// Error tracking
		std::string _last_error;
		int _last_errno = 0;

	private:
		Result performHandshake();
		Result flushSendQueue();
		bool shouldSendPing(int64_t now_ms);

	public:
		WebsocketClient();
		virtual ~WebsocketClient();

		WebsocketClient(const WebsocketClient&) = delete;
		WebsocketClient& operator=(const WebsocketClient&) = delete;

		virtual Result connect(const std::string& host, int port, bool insecure = false);
		virtual Result tick(int64_t now_ms);
		virtual void close();

		virtual bool isConnected() const;
		virtual bool isConnecting() const;
		virtual bool isOpen() const;

		virtual WsState state() const { return _state; }

		virtual IoResult sendText(const std::string& text);
		virtual IoResult sendPing();
		virtual IoResult sendPong();

		virtual IoResult recvFrame(WebSocketFrame& out_frame);

		virtual std::string lastErrorString() const;
		virtual int lastErrno() const;

		virtual std::size_t sendQueueSize() const;
};