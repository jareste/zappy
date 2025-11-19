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
signal tile_updated(x, y, object)
signal team_updated(team_name)

func update_game_state(json_data):
	"""Update the entire game state from JSON"""
	print("DataManager: Updating game state with keys: ", json_data.keys())
	game_data = json_data
	
	if json_data.has("map"):
		print("DataManager: Processing map data...")
		_update_map_data(json_data.map)
	
	if json_data.has("players"):
		print("DataManager: Processing players data...")
		_update_players_data(json_data.players)
	
	if json_data.has("eggs"):
		print("DataManager: Processing eggs data...")
		_update_eggs_data(json_data.eggs)
	else:
		print("DataManager: No eggs data in JSON")
	
	if json_data.has("game"):
		print("DataManager: Processing game data...")
		_update_game_info(json_data.game)
	
	print("DataManager: Emitting game_state_updated signal...")
	game_state_updated.emit()

func _update_map_data(map_data):
	print("DataManager: Map data - width: ", map_data.width, ", height: ", map_data.height)
	print("DataManager: Tiles count: ", map_data.tiles.size())
	map_size.x = map_data.width
	map_size.y = map_data.height
	
	# Clear existing tiles data
	tiles.clear()
	
	# Update tiles
	var tile_id = 0
	for tile_data in map_data.tiles:
		var pos = Vector2i(tile_data.x, tile_data.y)
		
		var player_ids = []
		for player_id in tile_data.players:
			player_ids.append(int(player_id))
		
		var egg_ids = []
		if tile_data.has("eggs"):
			for egg_id in tile_data.eggs:
				egg_ids.append(int(egg_id))
		
		tiles[pos] = {
			"id": tiles.size(),
			"position": pos,
			"resources": tile_data.resources,
			"players": player_ids,
			"eggs": egg_ids
		}
		tile_updated.emit(tile_data.x, tile_data.y, "MAP_DATA_UPDATE")

func _update_players_data(players_data):
	players.clear()
	for player_data in players_data:
		var player_id = int(player_data.id)  # Convert to integer
		players[player_id] = {
			"id": player_id,
			"position": Vector2i(int(player_data.position.x), int(player_data.position.y)),
			"orientation": int(player_data.orientation),
			"level": int(player_data.level),
			"team": player_data.team,
			"inventory": player_data.inventory,
			"status": player_data.status
		}
		player_updated.emit(player_id)

func _update_eggs_data(eggs_data):
	eggs.clear()
	for egg_data in eggs_data:
		var egg_id = int(egg_data.id)
		eggs[egg_id] = {
			"id": egg_id,
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

	MockServer.set_time_unit_value(game_info.time_unit)
	
	# Update teams data
	teams.clear()
	for team_data in game_data_info.teams:
		teams[team_data.name] = {
			"player_count": team_data.player_count,
			"remaining_connections": team_data.remaining_connections
		}
		team_updated.emit(team_data.name)

func get_tile_data(x: int, y: int):
	var pos = Vector2i(x, y)
	return tiles.get(pos, null)

func get_player_data(player_id: int):
	return players.get(player_id, null)

func get_team_data(team_name: String):
	return teams.get(team_name, null)

func get_egg_data(egg_id: int):
	return eggs[egg_id]

func get_resource_total(resource_name: String) -> int:
	var total = 0
	for tile_pos in tiles:
		var tile = tiles[tile_pos]
		if tile.resources.has(resource_name):
			total += tile.resources[resource_name]
	return total
