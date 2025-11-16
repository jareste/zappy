# PlayerManager.gd - Manages the visual representation of players
extends Node3D

@onready var ui: Control = $"../UI"
@onready var player_root: Node3D
@onready var world_manager: Node3D
@onready var egg_root: Node3D

var players = {}
var eggs= {}

# Configuration constants
const PLAYER_HEIGHT = 0
var world_is_ready = false
var players_created = false

# Team colors configuration
var team_colors = {
	"Alpha": Color.RED,
	"Beta": Color.BLUE,
	"Gamma": Color.GREEN,
	"Delta": Color.YELLOW
}

func _ready():
	GameData.connect("game_state_updated", _on_game_state_updated)
	GameData.connect("player_updated", _on_player_updated)
	GameData.connect("egg_updated", _on_egg_updated)

func initialize(player_root_node: Node3D, world_manager_ref: Node3D, egg_root_node: Node3D):
	"""Initialize the player manager with required node references"""
	player_root = player_root_node
	world_manager = world_manager_ref
	egg_root = egg_root_node

	if world_manager.has_signal("world_ready"):
		world_manager.connect("world_ready", _on_world_ready)

func _on_game_state_updated():
	"""Update all players when game state changes"""
	if world_is_ready:
		_update_all_players()
		_update_all_eggs()

func _on_player_updated(player_id: int):
	"""Update a specific player when their data changes"""
	_update_player_visual(player_id)

func _on_egg_updated(egg_id: int):
	"""Update a specific egg when their data changes"""
	_update_egg_visual(egg_id)

func _on_world_ready():
	"""Called when the world is ready - now we can safely place players"""
	world_is_ready = true
	players_created = false
	_update_all_players()
	_update_all_eggs()

func _setup_player_hover_signals(player_scene, player_id: int):
	var player_area = player_scene.find_child("Area3D") as Area3D
	
	if player_area.mouse_entered.is_connected(_on_player_area_mouse_entered):
		player_area.mouse_entered.disconnect(_on_player_area_mouse_entered)
	
	player_area.mouse_entered.connect(_on_player_area_mouse_entered.bind(player_id))
	player_area.mouse_exited.connect(_on_player_area_mouse_exited.bind())

func _on_player_area_mouse_entered(player_id: int):
	ui.update_ui_player_stats(player_id)

func _on_player_area_mouse_exited():
	ui.hide_player_info()

func _update_all_players():
	"""Update all player visuals"""	
	if not player_root or not world_is_ready or players_created:
		return
	
	players_created = true
		
	for child in player_root.get_children():
		child.queue_free()
	players.clear()
	
	for player_id in GameData.players:
		_create_player_visual(player_id)
		
func _update_all_eggs():
	if not egg_root:
		return

	for child in egg_root.get_children():
		child.queue_free()
	eggs.clear()

	for egg_id in GameData.eggs:
		_create_egg_visual(egg_id)
	

func _create_egg_visual(egg_id: int):
	"""Create visual representation for egg"""
	var egg_data = GameData.get_egg_data(egg_id)

	# WILL NEED TO HANDLE DIFFERENT EGG STATES
	if not egg_data or not egg_root or not world_manager:
		print("me fui papa")
		return

	var egg_scene = preload("res://scenes/resources/eggResource.tscn").instantiate()
	var pos = egg_data.position
	var world_pos = world_manager.get_world_position(pos.x, pos.y)

	egg_scene.get_node("egg").setup_egg_hover_signals(ui, egg_id)

	egg_scene.scale = Vector3(0.3, 0.3, 0.3)
	
	egg_scene.position = world_pos
	egg_root.add_child(egg_scene)
	eggs[egg_id] = egg_scene

func _update_egg_visual(_egg_id: int):
	pass

func _create_player_visual(player_id: int):
	"""Create visual representation for a player"""
	var player_data = GameData.get_player_data(player_id)
	if not player_data or not player_root or not world_manager:
		return
	
	var player_scene
	match player_data.team:
		"Alpha":
			player_scene = preload("res://scenes/player/playerDody.tscn").instantiate()
		"Beta":
			player_scene = preload("res://scenes/player/playerPiry.tscn").instantiate()
		"Gamma":
			player_scene = preload("res://scenes/player/playerBally.tscn").instantiate()
		"Delta":
			player_scene = preload("res://scenes/player/playerCuby.tscn").instantiate()
		_:
			player_scene = preload("res://scenes/player/playerIcy.tscn").instantiate()
	if not player_scene:
		return
	
	var pos = Vector2i(player_data.position.x, player_data.position.y)
	
	if world_is_ready and world_manager.tiles.has(pos):
		var tile_scene = world_manager.tiles[pos]
		if tile_scene:
			place_player_in_tile(tile_scene, player_scene, player_id)
		else:
			player_root.add_child(player_scene)
			var world_pos = world_manager.get_world_position(pos.x, pos.y)
			world_pos.y = PLAYER_HEIGHT - 0.3
			player_scene.position = world_pos

	else:
		player_root.add_child(player_scene)
		var world_pos = world_manager.get_world_position(pos.x, pos.y)
		world_pos.y = PLAYER_HEIGHT - 0.3
		player_scene.position = world_pos

	players[player_id] = player_scene
	
	_update_player_visual(player_id)
	_setup_player_hover_signals(players[player_id], player_id)

func place_player_in_tile(tile_scene: Area3D, player_scene: Node3D, player_id: int):
	var position_index = tile_scene.get_player_available_position_index()

	if position_index == -1:
		print("Warning: No available positions in tile for player")
		return

	tile_scene.occupy_player_position(position_index, player_scene, player_id)

func _update_player_visual(player_id: int):
	"""Update a specific player's visual representation"""
	if not players.has(player_id):
		_create_player_visual(player_id)
		return
	
	var player_data = GameData.get_player_data(player_id)
	if not player_data or not world_manager:
		return
	
	var player_scene = players[player_id]

	_update_player_color(player_scene, player_data)

func _update_player_color(_player_scene: Node3D, _player_data: Dictionary):
	pass

func get_player_count() -> int:
	"""Get the current number of players"""
	return players.size()

func get_player_scene(player_id: int) -> Node3D:
	"""Get the scene node for a specific player"""
	return players.get(player_id, null)
