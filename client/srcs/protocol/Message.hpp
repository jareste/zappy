#pragma once

#include <vector>
#include <string>
#include <optional>

enum class Orientation {
	N = 0, E = 1, S = 2, W = 3
};


struct VisionTile {
	int							distance;
	int							localX;
	int							localY;
	int							playerCount;
	std::vector<std::string>	items;

	bool hasItem(const std::string& item) const {
		for (auto tileItem : items) {
			if (tileItem == item) return true;
		}
		return false;
	}

	int countItem(const std::string& item) const {
		int count = 0;
		for (auto tileItem : items) {
			if (tileItem == item) count++;
		}
		return count;
	}
};

struct Inventory {
	int nourriture	= 0;
	int linemate	= 0;
	int deraumere	= 0;
	int sibur		= 0;
	int mendiane	= 0;
	int phiras		= 0;
	int thystame	= 0;
};

enum class MsgType {
	Unknown, Bienvenue, Welcome, Response, Event, Broadcast, GameEnd, Error
};

struct ServerMessage {
	MsgType		type = MsgType::Unknown;
	std::string	raw;

	std::string winnerTeam;
    int			winnerTeamId = -1;

	// for response
	std::string cmd;
	std::string arg;
	std::string status;

	// for welcome
	std::optional<int>			mapWidth;
	std::optional<int>			mapHeight;
	std::optional<int>			remainingSlots;

	std::optional<int>			playerX;
	std::optional<int>			playerY;
	std::optional<Orientation>	playerOrientation;

	// for voir
	std::optional<std::vector<VisionTile>> vision;

	// for inventaire
	std::optional<Inventory> inventory;

	// for broadcast
	std::optional<std::string> messageText;
	std::optional<int> broadcastDirection;

	// for connect_nrb
	std::optional<int> connectNbr;

	bool isOk()         const { return status == "ok"; }
	bool isKo()         const { return status == "ko"; }
	bool isInProgress() const { return status == "in_progress"; }
	bool isDeath()      const { return status == "died"; }
	bool isLevelUp()    const { return status == "level_up"; }
};
