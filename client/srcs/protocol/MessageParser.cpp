#include "MessageParser.hpp"
#include "../helpers/Logger.hpp"
#include "../../incs/third_party/json.hpp"

#include <algorithm>
#include <cmath>

using json = nlohmann::json;

ServerMessage MessageParser::parse(const std::string& raw) {
	ServerMessage msg;
	msg.raw = raw;

	// parse the raw string received through the socket
	json root;
	try {
		root = json::parse(raw);
	} catch(const json::exception &e) {
		Logger::error("Failed to parse JSON: " + std::string(e.what()));
		return msg;
	}

	// get the msg type
	if (root.contains("type") && root.at("type").is_string()) {
		std::string typeStr = root.at("type").get<std::string>();
		if		(typeStr == "bienvenue")	msg.type = MsgType::Bienvenue;
		else if	(typeStr == "welcome")		msg.type = MsgType::Welcome;
		else if	(typeStr == "response")		msg.type = MsgType::Response;
		else if	(typeStr == "error")		msg.type = MsgType::Error;
		else if	(typeStr == "event")		msg.type = MsgType::Event;
		else if	(typeStr == "message")		msg.type = MsgType::Broadcast;
		else if (typeStr == "game_end")		msg.type = MsgType::GameEnd;
	}

	// get common fields
	if (root.contains("cmd") && root.at("cmd").is_string())
		msg.cmd = root.at("cmd").get<std::string>();
	if (root.contains("arg") && root.at("arg").is_string())
		msg.arg = root.at("arg").get<std::string>();
	if (root.contains("status") && root.at("status").is_string())
		msg.status = root.at("status").get<std::string>();

	// welcome case
	if (msg.type == MsgType::Welcome) {
		if (root.contains("remaining_clients") && root.at("remaining_clients").is_number())
			msg.remainingSlots = root.at("remaining_clients").get<int>();

		if (root.contains("x") && root.at("x").is_number())
			msg.playerX = root.at("x").get<int>();

		if (root.contains("y") && root.at("y").is_number())
			msg.playerY = root.at("y").get<int>();
			
		if (root.contains("orientation") && root.at("orientation").is_number())
			msg.playerOrientation = static_cast<Orientation>(root.at("orientation").get<int>());

		if (root.contains("map_size")) {
			const auto& ms = root.at("map_size");
			if (ms.contains("x") && ms.contains("y")) {
				msg.mapWidth = ms.at("x").get<int>();
				msg.mapHeight = ms.at("y").get<int>();
			}
		}
	}

	// vision/voir case
	if (msg.type == MsgType::Response && msg.cmd == "voir") {
		if (msg.isKo()) {
			Logger::warn("Failed to execute voir command");
			return msg;
		}

		if (root.contains("vision") && root.at("vision").is_array()) {
			std::vector<VisionTile> tiles;
			const auto& visionArr = root.at("vision");

			int currentLevel	= 0;
			int tilesInLevel	= 1;
			int tilesProcessed	= 0;

			for (const auto& tileArr : visionArr) {
				if (tilesProcessed >= tilesInLevel) {
					currentLevel++;
					tilesInLevel = 2 * currentLevel + 1;
					tilesProcessed = 0;
				}

				if (tileArr.is_array()) {
					VisionTile tile;
					tile.distance = currentLevel;
					tile.localX = tilesProcessed - currentLevel;
					tile.localY = currentLevel;

					for (const auto& item : tileArr) {
						if (item.is_string()) {
							std::string s = item.get<std::string>();
							if (s == "player") {
								tile.playerCount++;
							} else {
								tile.items.push_back(s);
							}
						}
					}
					tiles.push_back(tile);
				}
				tilesProcessed++;
			}
			msg.vision = tiles;
			Logger::debug("Parsed " + std::to_string(tiles.size()) + " vision tiles");
		}
	}

	// inventory case
	if (msg.type == MsgType::Response && msg.cmd == "inventaire") {
		if (root.contains("inventaire") && root.at("inventaire").is_object()) {
			const auto& inv = root.at("inventaire");
			Inventory inventory;

			if (inv.contains("nourriture")) inventory.nourriture = inv.at("nourriture").get<int>();
			if (inv.contains("linemate"))   inventory.linemate   = inv.at("linemate").get<int>();
			if (inv.contains("deraumere"))  inventory.deraumere  = inv.at("deraumere").get<int>();
			if (inv.contains("sibur"))      inventory.sibur      = inv.at("sibur").get<int>();
			if (inv.contains("mendiane"))   inventory.mendiane   = inv.at("mendiane").get<int>();
			if (inv.contains("phiras"))     inventory.phiras     = inv.at("phiras").get<int>();
			if (inv.contains("thystame"))   inventory.thystame   = inv.at("thystame").get<int>();
			msg.inventory = inventory;
		}
	}

	// broadcast case
	if (msg.type == MsgType::Broadcast) {
		if (!msg.arg.empty())
			msg.messageText = msg.arg;
		if (!msg.status.empty())
			try {
				msg.broadcastDirection = std::stoi(msg.status);
			} catch (...) {
				Logger::warn("Failed to parse message direction: " + msg.status);
			}
	}

	if (msg.type == MsgType::GameEnd) {
		if (root.contains("winner_team") && root["winner_team"].is_string())
			msg.winnerTeam = root["winner_team"].get<std::string>();
		if (root.contains("winner_team_id") && root["winner_team_id"].is_number())
			msg.winnerTeamId = root["winner_team_id"].get<int>();
	}

	// connect_nbr case
	if (msg.type == MsgType::Response && msg.cmd == "connect_nbr") {
		if (root.contains("arg") && root.at("arg").is_string())
			msg.connectNbr = std::stoi(root.at("arg").get<std::string>());
	}
	
	return msg;
}
