extends Node3D

@onready var ui: Control = $"../UI"
@onready var player_root: Node3D
@onready var world_manager: Node3D
@onready var egg_root: Node3D

var players = {}
var eggs = {}

const PLAYER_HEIGHT = 0
var world_is_ready = false
var player_logical_orientations = {} 

var team_colors = {
	"Alpha": Color.RED,
	"Beta": Color.BLUE, 
	"Gamma": Color.GREEN,
	"Delta": Color.YELLOW
}

func _ready():
	GameData.connect("game_state_updated", _on_game_state_updated)
	
func initialize(player_root_node: Node3D, world_manager_ref: Node3D, egg_root_node: Node3D):
	"""Initialize the player manager with required node references"""
	player_root = player_root_node
	world_manager = world_manager_ref
	egg_root = egg_root_node

	if world_manager.has_signal("world_ready"):
		world_manager.connect("world_ready", _on_world_ready)

	# Connect to CommandProcessor's signals
	CommandProcessor.connect("player_orientation_change", _on_player_orientation_change)
	CommandProcessor.connect("player_position_change", _on_player_position_change)
	CommandProcessor.connect("egg_laid", _on_egg_laid)

# Initialization
func _on_game_state_updated():
	"""Initial setup only - commands handle real-time updates"""
	if world_is_ready:
		_create_all_players()
		_create_all_eggs()

func _on_world_ready():
	"""Called when the world is ready"""
	world_is_ready = true
	_create_all_players()
	_create_all_eggs()

# Creation
func _create_all_players():
	"""Create all player visuals from scratch"""
	if not player_root or not world_is_ready:
		return
		
	# Clear existing
	for child in player_root.get_children():
		child.queue_free()
	players.clear()
	
	# Create from GameData
	for player_id in GameData.players:
		_create_player_visual(player_id)

func _create_all_eggs():
	"""Create all egg visuals from scratch"""
	if not egg_root:
		return

	for child in egg_root.get_children():
		child.queue_free()
	eggs.clear()

	for egg_id in GameData.eggs:
		_create_egg_visual(egg_id)

func _create_player_visual(player_id: int):
	"""Create visual representation for a player"""
	var player_data = GameData.get_player_data(player_id)
	if not player_data or not player_root or not world_manager:
		return
	
	var player_scene = _load_player_scene(player_data.team)
	if not player_scene:
		return
	
	var pos = Vector2i(player_data.position.x, player_data.position.y)
	
	player_root.add_child(player_scene)
	_place_player_at_position(player_scene, pos, player_id)
	
	players[player_id] = player_scene
	_setup_player_hover_signals(player_scene, player_id)

	player_logical_orientations[player_id] = player_data.orientation

func _create_egg_visual(egg_id: int):
	"""Create visual representation for egg"""
	var egg_data = GameData.get_egg_data(egg_id)
	if not egg_data or not egg_root or not world_manager:
		return

	var egg_scene = preload("res://scenes/resources/eggResource.tscn").instantiate()
	var pos = egg_data.position
	var world_pos = world_manager.get_world_position(pos.x, pos.y)

	egg_scene.get_node("egg").setup_egg_hover_signals(ui, egg_id)
	egg_scene.scale = Vector3(0.3, 0.3, 0.3)
	egg_scene.position = world_pos
	
	egg_root.add_child(egg_scene)
	eggs[egg_id] = egg_scene

# Command Handlers
func _on_player_orientation_change(player_id: int, new_orientation: int):
	"""Handle real-time orientation changes from commands"""
	var player_scene = players.get(player_id)
	if not player_scene:
		return
	
	var anim_player = _find_animation_player(player_scene)
	if not anim_player:
		print("Warning: AnimationPlayer not found for player ", player_id)
		return
	
	if anim_player.current_animation != "idle":
		anim_player.play("idle")
	
	var test_cuby = player_scene.get_node("test_cuby")
	var old_orientation = player_logical_orientations.get(player_id, new_orientation)
	
	var turn_amount = _calculate_turn_amount(old_orientation, new_orientation)
	var current_rotation = test_cuby.rotation.y
	var target_rotation = current_rotation + turn_amount
	
	player_logical_orientations[player_id] = new_orientation
	
	var tween = create_tween()
	tween.set_ease(Tween.EASE_OUT)
	tween.set_trans(Tween.TRANS_QUART)
	tween.tween_property(test_cuby, "rotation:y", target_rotation, 0.3)
	
	print("Rotating player ", player_id, " from ", old_orientation, " to ", new_orientation, " (turn: ", rad_to_deg(turn_amount), "°)")

func _find_animation_player(player_scene: Node3D) -> AnimationPlayer:
	"""Find AnimationPlayer in the player scene hierarchy"""
	var test_cuby = player_scene.get_node("test_cuby")
	if test_cuby:
		return test_cuby.find_child("AnimationPlayer", true, false)
	return null

func _calculate_turn_amount(from_orientation: int, to_orientation: int) -> float:
	"""Calculate the rotation amount for the turn"""
	var turn = (to_orientation - from_orientation + 4) % 4
	
	match turn:
		1: return deg_to_rad(90)
		2: return deg_to_rad(180)
		3: return deg_to_rad(-90)
		_: return 0.0 

func _on_player_position_change(player_id: int, current_orientation: int, player_data: Dictionary, movement_length: float):
	"""Handle real-time position changes from commands"""
	var player_scene = players.get(player_id)
	if not player_scene:
		print("Warning: Player ", player_id, " not found in visual players")
		return
	
	var old_pos = Vector2i(player_data.position.x, player_data.position.y)
	var new_pos = _calculate_new_position(old_pos, current_orientation)
	
	# Update data
	player_data.position.x = new_pos.x
	player_data.position.y = new_pos.y
	
	# Update tile data
	_update_tile_player_data(player_id, old_pos, new_pos)
	GameData.tile_updated.emit(old_pos.x, old_pos.y, "PLAYER_EXIT")
	GameData.tile_updated.emit(new_pos.x, new_pos.y, "PLAYER_ENTER")

	_move_player_visual(player_scene, old_pos, new_pos, current_orientation)

func _move_player_visual(player_scene: Node3D, old_pos: Vector2i, new_pos: Vector2i, orientation: int):
	"""Smoothly move player visual between positions"""
	var current_world_pos = player_scene.global_position
	var target_world_pos = _calculate_target_world_position(old_pos, new_pos, orientation, current_world_pos)
	
	# Create smooth movement tween
	var tween = create_tween()
	tween.set_ease(Tween.EASE_IN)
	tween.set_ease(Tween.EASE_OUT)
	tween.set_trans(Tween.TRANS_QUART)
	tween.tween_property(player_scene, "global_position", target_world_pos, 1)
	
	print("Moving player from ", old_pos, " to ", new_pos, " (", current_world_pos, " → ", target_world_pos, ")")

func _calculate_target_world_position(old_pos: Vector2i, new_pos: Vector2i, orientation: int, current_pos: Vector3) -> Vector3:
	"""Calculate the target world position handling wrapping"""
	var target_pos = current_pos
	var tile_size = world_manager.tile_size if world_manager.has_method("get_tile_size") else 1.0
	
	match orientation:
		1: # North
			if new_pos.y == 0 and old_pos.y == GameData.map_size.y - 1:
				target_pos.z = current_pos.z - (GameData.map_size.y * tile_size - 1)
			else:
				target_pos.z = current_pos.z + tile_size
		
		2: # East  
			if new_pos.x == 0 and old_pos.x == GameData.map_size.x - 1:
				target_pos.x = current_pos.x - (GameData.map_size.x * tile_size - 1)
			else:
				target_pos.x = current_pos.x + tile_size
		
		3: # South
			if new_pos.y == GameData.map_size.y - 1 and old_pos.y == 0:
				target_pos.z = current_pos.z + (GameData.map_size.y * tile_size - 1)
			else:
				target_pos.z = current_pos.z - tile_size
		
		4: # West
			if new_pos.x == GameData.map_size.x - 1 and old_pos.x == 0:
				target_pos.x = current_pos.x + (GameData.map_size.x * tile_size - 1)
			else:
				target_pos.x = current_pos.x - tile_size
	
	return target_pos

func _on_egg_laid(egg_data: Dictionary) -> void:
	GameData.eggs[egg_data.id] = egg_data
	_create_egg_visual(egg_data.id)

# Helpers
func _load_player_scene(team: String) -> Node3D:
	"""Load appropriate player scene based on team"""
	match team:
		"Alpha": return preload("res://scenes/player/playerCuby.tscn").instantiate()
		"Beta": return preload("res://scenes/player/playerPiry.tscn").instantiate()
		"Gamma": return preload("res://scenes/player/playerBally.tscn").instantiate()
		"Delta": return preload("res://scenes/player/playerDody.tscn").instantiate()
		_: return preload("res://scenes/player/playerIcy.tscn").instantiate()

func _place_player_at_position(player_scene: Node3D, pos: Vector2i, player_id: int):
	"""Place player at specific tile position"""
	if world_is_ready and world_manager.tiles.has(pos):
		var tile_scene = world_manager.tiles[pos]
		if tile_scene:
			_place_player_in_tile(tile_scene, player_scene, player_id)
			return
	
	# Fallback
	var world_pos = world_manager.get_world_position(pos.x, pos.y)
	world_pos.y = PLAYER_HEIGHT - 0.3
	player_scene.position = world_pos

func _place_player_in_tile(tile_scene: Area3D, player_scene: Node3D, player_id: int):
	"""Place player in available tile slot"""
	var position_index = tile_scene.get_player_available_position_index()
	if position_index == -1:
		print("Warning: No available positions in tile for player")
		return
	tile_scene.occupy_player_position(position_index, player_scene, player_id)

func _rotate_player(player_scene: Node3D, new_orientation: int):
	"""Rotate player to new orientation"""
	var target_rotation = deg_to_rad((new_orientation - 1) * 90.0)
	player_scene.rotation.y = target_rotation

func _calculate_new_position(old_pos: Vector2i, orientation: int) -> Vector2i:
	"""Calculate new position based on orientation and world wrapping"""
	var new_pos = Vector2i(old_pos.x, old_pos.y)
	
	match orientation:
		1: # North
			new_pos.y = (old_pos.y + 1) % GameData.map_size.y
		2: # East
			new_pos.x = (old_pos.x + 1) % GameData.map_size.x
		3: # South
			new_pos.y = (old_pos.y - 1 + GameData.map_size.y) % GameData.map_size.y
		4: # West
			new_pos.x = (old_pos.x - 1 + GameData.map_size.x) % GameData.map_size.x
	
	return new_pos

func _update_tile_player_data(player_id: int, old_pos: Vector2i, new_pos: Vector2i):
	"""Update tile player lists"""
	var old_tile = GameData.tiles.get(old_pos)
	if old_tile and old_tile.players.has(player_id):
		old_tile.players.erase(player_id)
		GameData.tile_updated.emit(old_pos.x, old_pos.y, "PLAYER_ERASE")
	
	var new_tile = GameData.tiles.get(new_pos)
	if new_tile:
		if not new_tile.players.has(player_id):
			new_tile.players.append(player_id)
		GameData.tile_updated.emit(new_pos.x, new_pos.y, "PLAYER_ADD")

func _setup_player_hover_signals(player_scene: Node3D, player_id: int):
	"""Setup mouse hover for player"""
	var player_area = player_scene.find_child("Area3D") as Area3D
	if player_area:
		player_area.mouse_entered.connect(_on_player_area_mouse_entered.bind(player_id))
		player_area.mouse_exited.connect(_on_player_area_mouse_exited)

func _on_player_area_mouse_entered(player_id: int):
	ui.update_ui_player_stats(player_id)

func _on_player_area_mouse_exited():
	ui.hide_player_info()

# Interface
func get_player_count() -> int:
	return players.size()

func get_player_scene(player_id: int) -> Node3D:
	return players.get(player_id, null)
