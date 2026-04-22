#pragma once

#include "State.hpp"
#include "Behavior.hpp"
#include "../net/WebsocketClient.hpp"
#include "../protocol/MessageParser.hpp"
#include "../protocol/Sender.hpp"
#include "../net/FrameCodec.hpp"
#include "../../incs/Result.hpp"

#include <string>
#include <atomic>
#include <thread>

class Agent {
	private:
		std::string _host;
		int         _port;
		std::string _teamName;
		std::string	_key;
		bool _forkEnabled = true;

		WebsocketClient	_ws;
		Sender			_sender;
		WorldState		_state;
		Behavior		_behavior;

		std::atomic<bool>				_running{false};
		std::unique_ptr<std::thread>	_networkThread;

		bool _victory = false;

		static constexpr int MAX_RECONNECT				= 5;
		static constexpr int COMMAND_FLIGHT_TIMEOUT_MS	= 3000;


		void networkLoop();
		void processIncomingMessages(int64_t nowMs);
		Result waitForBienvenue(int64_t& nowMs, int timeoutMs);
		Result performLogin(int64_t& nowMs, int timeoutMs);
	
	public:
		static constexpr int CONNECT_TIMEOUT_MS			= 10000;

		Agent(const std::string& host, const int port, const std::string& teamName, const std::string& key);
		~Agent();

		void setEasyMode(bool enabled) { _behavior.setEasyMode(enabled); }

		bool isRunning() const { return _running; }
		const WorldState& getState() const { return _state; }
		void setForkEnabled(bool enabled) { _forkEnabled = enabled; _state.captureForkStatus(enabled); }

		Result connect(int timeoutMs);
		Result run();
		void stop();
};