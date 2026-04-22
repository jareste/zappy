#pragma once

#include "../net/WebsocketClient.hpp"
#include "Message.hpp"

#include <functional>
#include <chrono>
#include <deque>

class Sender {
	public:
		struct PendingCommand {
			std::string									cmd;
			std::chrono::steady_clock::time_point		sentAt;
			std::function<void(const ServerMessage&)>	callback;
		};

	private:
		WebsocketClient&			_ws;
		std::deque<PendingCommand>	_pending;

		// helper
		Result sendObject(const std::string& dump);

	public:
		explicit Sender(WebsocketClient& ws);
		~Sender() = default;

		Result sendLogin(const std::string& teamName, const std::string& key="SOME_KEY");

		virtual Result sendVoir();
		virtual Result sendInventaire();
		
		virtual Result sendAvance();
		virtual Result sendDroite();
		virtual Result sendGauche();

		virtual Result sendPrend(const std::string& resource);
		Result sendPose(const std::string& resource);

		Result sendBroadcast(const std::string& msg);
		
		Result sendIncantation();
		Result sendFork();
		Result sendConnectNbr();

		Result sendClaimLeader();
		Result sendDisbandLeader();

		virtual void expect(const std::string& cmd, std::function<void(const ServerMessage&)> callback);
		void processResponse(const ServerMessage& msg);
		void checkTimeouts(int timeoutMs = 10000);

		void cancelAll();
};