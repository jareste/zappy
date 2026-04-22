#pragma once

#include "../protocol/Message.hpp"

#include <iostream>
#include <cmath>

struct PlayerState {
	int			x = 0;
	int			y = 0;
	Orientation	orientation = Orientation::N;
	int			level = 1;
	Inventory	inventory;
	int			remainingSlots = 0;

	int food() const { return inventory.nourriture; }
};

struct WorldState {
	PlayerState				player;
	std::vector<VisionTile>	vision;
	int						mapWidth = 0;
	int						mapHeight = 0;
	bool					forkEnabled = false;

	std::string serverHost;
	int serverPort;

	// NOTE: server does not send orientation in welcome; player starts at N by default.
	// If the server ever adds this field, the optional handles it correctly.
	void onWelcome(const ServerMessage& msg) {
		std::cout << msg.raw << std::endl;
		if (msg.playerX.has_value())
			player.x = msg.playerX.value();

		if (msg.playerY.has_value())
			player.y = msg.playerY.value();
			
		if (msg.playerOrientation.has_value())
			player.orientation = msg.playerOrientation.value();
		
		if (msg.remainingSlots.has_value())
			player.remainingSlots = msg.remainingSlots.value();

		if (msg.mapWidth.has_value())
			mapWidth = msg.mapWidth.value();
		
		if (msg.mapHeight.has_value())
			mapHeight = msg.mapHeight.value();
	}

	// helper queries
	bool visionHasItem(const std::string& item) const {
		for (auto& tile : vision) {
			for (auto& tileItem : tile.items) {
				if (tileItem == item)
				 return true;
			}
		}
		return false;
	}

	std::optional<VisionTile> nearestTileWithItem(const std::string& item) const {
		std::optional<VisionTile> bestTile = std::nullopt;
		for (const auto& tile : vision) {
			if (bestTile.has_value() && tile.distance > bestTile->distance) {
				break;
			}
			for (const auto& tileItem : tile.items) {
				if (tileItem == item) {
					if (!bestTile.has_value() || std::abs(tile.localX) < std::abs(bestTile->localX)) {
						bestTile = tile;
					}
					break;
				}
			}
		}
		return bestTile;
	}

	int playersOnCurrentTile() const {
		if (!vision.empty()) {
			return vision[0].playerCount;
		}
		return 0;
	}

	int countItemOnCurrentTile(const std::string& item) const {
		if (!vision.empty()) {
			return vision[0].countItem(item);
		}
		return 0;
	}

	void captureForkStatus(bool status) { forkEnabled = status; }
};