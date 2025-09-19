# GameData.gd - Singleton to manage game state
extends Node

# Game state data structures
var game_data = {}
var map_size = Vector2i(0, 0)
var tiles = {}
var players = {}
var eggs = {}
var teams = {}
var game_info = {}

# Resource types for easy reference
var RESOURCES = [
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

# Signals for UI updates
signal game_state_updated
signal player_updated(player_id)
signal tile_updated(x, y)
signal team_updated(team_name)

func update_game_state(json_data):
	"""Update the entire game state from JSON"""
	game_data = json_data
	
	if json_data.has("map"):
		_update_map_data(json_data.map)
	
	if json_data.has("players"):
		_update_players_data(json_data.players)
	
	if json_data.has("eggs"):
		_update_eggs_data(json_data.eggs)
	
	if json_data.has("game"):
		_update_game_info(json_data.game)
	
	emit_signal("game_state_updated")

func _update_map_data(map_data):
	map_size.x = map_data.width
	map_size.y = map_data.height
	
	# Clear existing tiles data
	tiles.clear()
	
	# Update tiles
	for tile_data in map_data.tiles:
		var pos = Vector2i(tile_data.x, tile_data.y)
		tiles[pos] = {
			"resources": tile_data.resources,
			"players": tile_data.players,
			"eggs": tile_data.eggs
		}
		emit_signal("tile_updated", tile_data.x, tile_data.y)

func _update_players_data(players_data):
	players.clear()
	for player_data in players_data:
		var player_id = int(player_data.id)  # Convert to integer
		players[player_id] = {
			"position": Vector2i(int(player_data.position.x), int(player_data.position.y)),
			"orientation": int(player_data.orientation),
			"level": int(player_data.level),
			"team": player_data.team,
			"inventory": player_data.inventory,
			"status": player_data.status
		}
		emit_signal("player_updated", player_id)

func _update_eggs_data(eggs_data):
	eggs.clear()
	for egg_data in eggs_data:
		eggs[egg_data.id] = {
			"position": Vector2i(egg_data.position.x, egg_data.position.y),
			"status": egg_data.status,
			"parent_id": egg_data.parent_id,
			"team": egg_data.team
		}

func _update_game_info(game_data_info):
	game_info = {
		"tick": game_data_info.tick,
		"time_unit": game_data_info.time_unit,
		"teams": {}
	}
	
	# Update teams data
	teams.clear()
	for team_data in game_data_info.teams:
		teams[team_data.name] = {
			"player_count": team_data.player_count,
			"remaining_connections": team_data.remaining_connections
		}
		emit_signal("team_updated", team_data.name)

func get_tile_data(x: int, y: int):
	var pos = Vector2i(x, y)
	return tiles.get(pos, null)

func get_player_data(player_id: int):
	return players.get(player_id, null)

func get_team_data(team_name: String):
	return teams.get(team_name, null)

func get_resource_total(resource_name: String) -> int:
	var total = 0
	for tile_pos in tiles:
		var tile = tiles[tile_pos]
		if tile.resources.has(resource_name):
			total += tile.resources[resource_name]
	return total
