# GameConfig.gd - Configuration and constants for the Zappy GUI
# This should be set as AutoLoad singleton
extends Node

# Visual settings
const TILE_SIZE = 1.0
const TILE_GAP = 0.1
const PLAYER_HEIGHT = 0.5
const MOVEMENT_TWEEN_DURATION = 0.3

# Team colors
const TEAM_COLORS = {
	"Alpha": Color.RED,
	"Beta": Color.BLUE,
	"Gamma": Color.GREEN,
	"Delta": Color.YELLOW
}

# Resource colors for tiles
const RESOURCE_COLORS = {
	"nourriture": Color.GREEN,
	"linemate": Color.YELLOW,
	"deraumere": Color.BLUE,
	"sibur": Color.RED,
	"mendiane": Color.PURPLE,
	"phiras": Color.ORANGE,
	"thystame": Color.CYAN
}

# Network settings
const DEFAULT_SERVER_HOST = "127.0.0.1"
const DEFAULT_SERVER_PORT = 4242
const OBSERVER_COMMAND = "GRAPHIC\n"

# Resource types for easy reference
const RESOURCES = [
	"nourriture", "linemate", "deraumere", 
	"sibur", "mendiane", "phiras", "thystame"
]

# Player orientations
enum Orientation {
	NORTH = 1,
	EAST = 2, 
	SOUTH = 3,
	WEST = 4
}

# Player status effects
enum PlayerStatus {
	NORMAL,
	INCANTATION,
	BROADCASTING,
	DEAD
}

# Utility functions
static func get_spacing() -> float:
	return TILE_SIZE + TILE_GAP

static func get_team_color(team_name: String) -> Color:
	return TEAM_COLORS.get(team_name, Color.WHITE)

static func get_resource_color(resource_name: String) -> Color:
	return RESOURCE_COLORS.get(resource_name, Color.GRAY)

static func get_player_status_color_modifier(status: String, base_color: Color) -> Color:
	match status:
		"incantation":
			return base_color.lerp(Color.WHITE, 0.5)  # Lighter for incantation
		"broadcasting":
			return base_color.lerp(Color.YELLOW, 0.3)  # Yellowish for broadcasting
		_:
			return base_color  # Normal color
