#include "Sender.hpp"
#include "../helpers/Logger.hpp"
#include "../../incs/third_party/json.hpp"

using json = nlohmann::json;

Sender::Sender(WebsocketClient& ws) : _ws(ws) {}

// helper
Result Sender::sendObject(const std::string& dump) {
	IoResult res = _ws.sendText(dump);
	if (res.status != NetStatus::Ok)
		return Result::failure(ErrorCode::NetworkError, res.message);
	return Result::success();
}

// cmd handlers
Result Sender::sendLogin(const std::string& teamName, const std::string& key) {
	json login = {
		{"type", "login"},
		{"key", key.c_str()},
		{"role", "player"},
		{"team-name", teamName.c_str()},
	};

	return sendObject(login.dump());
}

Result Sender::sendVoir() {
	json voir = {
		{"type", "cmd"},
		{"cmd", "voir"}
	};

	return sendObject(voir.dump());
}

Result Sender::sendInventaire() {
	json inventaire = {
		{"type", "cmd"},
		{"cmd", "inventaire"}
	};

	return sendObject(inventaire.dump());
}
		
Result Sender::sendAvance() {
	json avance = {
		{"type", "cmd"},
		{"cmd", "avance"}
	};

	return sendObject(avance.dump());
}

Result Sender::sendDroite() {
	json droite = {
		{"type", "cmd"},
		{"cmd", "droite"}
	};

	return sendObject(droite.dump());
}

Result Sender::sendGauche() {
	json gauche = {
		{"type", "cmd"},
		{"cmd", "gauche"}
	};

	return sendObject(gauche.dump());
}

Result Sender::sendPrend(const std::string& resource) {
	json prend = {
		{"type", "cmd"},
		{"cmd", "prend"},
		{"arg", resource.c_str()}
	};

	return sendObject(prend.dump());
}

Result Sender::sendPose(const std::string& resource) {
	json pose = {
		{"type", "cmd"},
		{"cmd", "pose"},
		{"arg", resource.c_str()}
	};

	return sendObject(pose.dump());
}

Result Sender::sendBroadcast(const std::string& msg) {
	json broadcast = {
		{"type", "cmd"},
		{"cmd", "broadcast"},
		{"arg", msg.c_str()}
	};

	return sendObject(broadcast.dump());
}

Result Sender::sendIncantation() {
	json incantation = {
		{"type", "cmd"},
		{"cmd", "incantation"}
	};

	return sendObject(incantation.dump());
}

Result Sender::sendFork() {
	json fork = {
		{"type", "cmd"},
		{"cmd", "fork"}
	};

	return sendObject(fork.dump());
}

Result Sender::sendConnectNbr() {
	json connect = {
		{"type", "cmd"},
		{"cmd", "connect_nbr"}
	};

	return sendObject(connect.dump());
}

Result Sender::sendClaimLeader() {
	json j = {
		{"type", "cmd"},
		{"cmd", "claim_leader"}
	};
	return sendObject(j.dump());
}

Result Sender::sendDisbandLeader() {
	json j = {
		{"type", "cmd"},
		{"cmd", "disband_leader"}
	};
	return sendObject(j.dump());
}

// res tracking
void Sender::expect(const std::string& cmd,
		std::function<void(const ServerMessage&)> callback) {
	if (!callback) {
		Logger::warn("Sender::expect: null callback provided for command: " + cmd);
		return;
	}
	
	PendingCommand command;
	command.cmd = cmd;
	command.callback = callback;
	command.sentAt = std::chrono::steady_clock::now();
	
	_pending.push_back(command);

	Logger::debug("Sender: expectResponse for command: " + cmd);
}

void Sender::processResponse(const ServerMessage& msg) {
	if (msg.type != MsgType::Response)
		return;

	std::string lookupKey = msg.cmd;
	if (!msg.arg.empty() && (msg.cmd == "prend" || msg.cmd == "pose"))
		lookupKey += " " + msg.arg;

	if (msg.status == "in_progress" && msg.cmd == "incantation") {
		Logger::debug("Sender: incantation in_progress, forwarding but keeping pending");		
		auto it = std::find_if(_pending.begin(), _pending.end(),
			[](const PendingCommand& p) { return p.cmd == "incantation";});
		if (it != _pending.end() && it->callback) {
			it->callback(msg);
		}
		return;
	}

	auto it = std::find_if(_pending.begin(), _pending.end(),
		[&](const PendingCommand& p) { return p.cmd == lookupKey; });

	if (it != _pending.end()) {
		if (it->callback) {
			it->callback(msg);
		}
		_pending.erase(it);
		Logger::debug("Sender: matched pending '" + lookupKey + "'");
	} else {
		Logger::warn("Sender: no pending command for key='" + lookupKey + "' (msg.cmd='" + msg.cmd + "')");
	}
}

void Sender::checkTimeouts(int timeoutMs) {
	auto now = std::chrono::steady_clock::now();
	std::vector<std::pair<std::string, std::function<void(const ServerMessage&)>>> expired;

	for (auto it = _pending.begin(); it != _pending.end(); ) {
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->sentAt).count();

		if (elapsed >= timeoutMs) {
			Logger::warn("Sender: timeout on '" + it->cmd + "'");
			expired.emplace_back(it->cmd, it->callback);
			it = _pending.erase(it);
		} else {
			++it;
		}
	}

	for (const auto& [cmd, cb] : expired) {
		if (cb) {
			ServerMessage t;
			t.type = MsgType::Error;
			t.cmd = cmd;
			t.status = "timeout";
			cb(t);
		}
	}
}

// Cancels all pending commands, firing error callbacks so callers
// (e.g. Behavior) can reset their _commandInFlight flag and not deadlock.
void Sender::cancelAll() {
	std::vector<std::pair<std::string, std::function<void(const ServerMessage&)>>> cancelled;

	for (auto& p : _pending)
		cancelled.emplace_back(p.cmd, p.callback);
	_pending.clear();

	for (const auto& [cmd, cb] : cancelled) {
		if (cb) {
			ServerMessage t;
			t.type = MsgType::Error;
			t.cmd = cmd;
			t.status = "cancelled";
			cb(t);
		}
	}
}
