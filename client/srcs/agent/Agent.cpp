#include "Agent.hpp"
#include "../helpers/Logger.hpp"


Agent::Agent(const std::string& host, const int port, const std::string& teamName, const std::string& key)
	: _host(host), _port(port), _teamName(teamName), _key(key),
	 _sender(_ws), _behavior(_sender, _state, _teamName) {
		_state.serverHost = host;
		_state.serverPort = port;
	 }

Agent::~Agent() {
	stop();
	_ws.close();
}

Result Agent::connect(int timeoutMs) {
	Logger::info("Connecting to " + _host + ":" + std::to_string(_port));

	Result res = _ws.connect(_host, _port, true);
	if (!res.ok()) return res;

	int64_t nowMs = 0;
	auto startTime = std::chrono::steady_clock::now();

	while (!_ws.isConnected()) {
		Result tickRes = _ws.tick(nowMs);
		if (!tickRes.ok())
			return Result::failure(ErrorCode::NetworkError, "Websocket setup failed: " + tickRes.message);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		nowMs += 10;

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - startTime).count();
		if (elapsed > timeoutMs)
			return Result::failure(ErrorCode::NetworkError, "Websocket connection timeout");
	}
	Logger::info("Websocket connected");

	res = waitForBienvenue(nowMs, timeoutMs);
	if (!res.ok()) return res;

	res = performLogin(nowMs, timeoutMs);
	if (!res.ok()) return res;

	Logger::info("Logged in as " + _teamName);
	return Result::success();
}

Result Agent::waitForBienvenue(int64_t& nowMs, int timeoutMs) {
	auto startTime = std::chrono::steady_clock::now();

	while (true) {
		Result tickRes = _ws.tick(nowMs);
		if (!tickRes.ok())
			return Result::failure(ErrorCode::NetworkError,
									"Websocket error waiting for bienvenue: " + tickRes.message);

		WebSocketFrame frame;
		IoResult io = _ws.recvFrame(frame);
		if (io.status == NetStatus::Ok && frame.opcode == WebSocketOpcode::Text) {
			std::string text(frame.payload.begin(), frame.payload.end());
			ServerMessage msg = MessageParser::parse(text);
			if (msg.type == MsgType::Bienvenue) {

				Logger::info("Bienvenue raw message->" + msg.raw);

				Logger::info("Received bienvenue");
				return Result::success();
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		nowMs += 50;

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - startTime).count();
		if (elapsed > timeoutMs)
			return Result::failure(ErrorCode::ProtocolError, "No bienvenue received");
	}
}

Result Agent::performLogin(int64_t& nowMs, int timeoutMs) {
	Result res = _sender.sendLogin(_teamName, _key);
	if (!res.ok()) return res;

	auto startTime = std::chrono::steady_clock::now();

	while (true) {
		Result tickRes = _ws.tick(nowMs);
		if (!tickRes.ok())
			return Result::failure(ErrorCode::NetworkError,
									"Websocket error during login: " + tickRes.message);

		WebSocketFrame frame;
		IoResult io = _ws.recvFrame(frame);
		if (io.status == NetStatus::Ok && frame.opcode == WebSocketOpcode::Text) {
            std::string text(frame.payload.begin(), frame.payload.end());
            ServerMessage msg = MessageParser::parse(text);

            if (msg.type == MsgType::Welcome) {
                _state.onWelcome(msg);

				Logger::info("Login raw message->" + msg.raw);
				Logger::info("Login successful");

                return Result::success();
            }
            if (msg.type == MsgType::Error)
                return Result::failure(ErrorCode::ProtocolError, "Login error: " + msg.raw);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        nowMs += 50;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed > timeoutMs)
            return Result::failure(ErrorCode::ProtocolError, "No welcome received");
	}
}

Result Agent::run() {
	_running = true;
	_networkThread = std::make_unique<std::thread>(&Agent::networkLoop, this);
	return Result::success();
}

void Agent::stop() {
	_running = false;
	if (_networkThread && _networkThread->joinable())
		_networkThread->join();
	_sender.cancelAll();
}

void Agent::networkLoop() {
	int64_t nowMs = 0;
	int64_t lastStatusTime = 0;
	int reconnectAttempts = 0;
	
	while (_running) {
		auto tickStart = std::chrono::steady_clock::now();
		
		nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();

		Result tickRes = _ws.tick(nowMs);
		if (!tickRes.ok()) {
			Logger::error("Websocket tick error: " + tickRes.message);
			_running = false;
			break;
		}

		processIncomingMessages(nowMs);

		if (!_ws.isConnected()) {
			if (reconnectAttempts >= MAX_RECONNECT) {
				Logger::error("Max reconnect attempts reached");
				break;
			}
			reconnectAttempts++;
			Logger::warn("Disconnected, reconnect attempt " +
							std::to_string(reconnectAttempts));
			std::this_thread::sleep_for(std::chrono::seconds(2));

			_sender.cancelAll();
			Result res = connect(CONNECT_TIMEOUT_MS);
			if (!res.ok()) {
				Logger::error("Reconnection failed: " + res.message);
				continue;
			} else {
				reconnectAttempts = 0;
				Logger::info("Reconnected");
				continue;
			}
		}

		_sender.checkTimeouts(COMMAND_FLIGHT_TIMEOUT_MS);

		_behavior.tick(nowMs);

		if (nowMs - lastStatusTime > 5000) {
			Logger::info("Status: level=" + std::to_string(_state.player.level) +
						" behavior=" + std::to_string(static_cast<int>(_behavior.getState())) + 
						" food=" + std::to_string(_state.player.inventory.nourriture) +
						" forks=NOT TRACKED");
			lastStatusTime = nowMs;
		}

		auto tickElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - tickStart).count();
		int sleepMs = std::max(0LL, 50LL - tickElapsed);
		
		if (sleepMs > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
		}
		// If sleepMs is 0 or negative, already beind SO NO SLEEP
	}

	_running = false;
	Logger::info("Network loop ended");
}

void Agent::processIncomingMessages(int64_t nowMs) {
	(void)nowMs;

	WebSocketFrame frame;
	IoResult io = _ws.recvFrame(frame);

	while (io.status == NetStatus::Ok) {
		if (frame.opcode == WebSocketOpcode::Text) {
			std::string text(frame.payload.begin(), frame.payload.end());
			Logger::debug("RX: " + text);

			ServerMessage msg = MessageParser::parse(text);

			if (msg.isDeath()) {
				Logger::error("Player died! Stopping...");
				_running = false;
				break;
			}

			switch (msg.type) {
				case MsgType::Response:
					_sender.processResponse(msg);
					break;

				case MsgType::Event:
					if (msg.status == "level_up") {
						_behavior.setPendingLevelUp(true);
					}
					break;

				case MsgType::Broadcast:
					_behavior.onBroadcast(msg);
					break;

				case MsgType::GameEnd:
					Logger::info("Game ended! Team " + msg.winnerTeam + " won!");
					_victory = true;
					_running = false;
					break;

				case MsgType::Error:
					Logger::error("Server error: " + msg.raw);

				default:
					break;
			}
		} else if (frame.opcode == WebSocketOpcode::Ping) {
			_ws.sendPong();
		} else if (frame.opcode == WebSocketOpcode::Close) {
			Logger::info("Received close frame");
			break;
		}

		io = _ws.recvFrame(frame);
	}
}