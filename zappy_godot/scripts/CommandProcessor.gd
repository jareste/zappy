extends Node

signal command_processed(command_type: String, player_id: int)
signal command_failed(command_type: String, error: String)
signal player_orientation_change(player_id, new_orientation)
signal player_position_change(player_id, current_orientation)
signal egg_laid(egg_id)

signal object_amount_change(position, object_name)

var tile_size: float
var gap: float
var command_queue: Array[Dictionary] = []
var data_ready: bool = false
var queue_timer: Timer
var processing_queue: bool = false

func _ready():
	GameData.connect("game_state_updated", _on_game_data_ready)

	queue_timer = Timer.new()
	queue_timer.wait_time = 0.5 
	queue_timer.timeout.connect(_process_next_queued_command)
	add_child(queue_timer)

func _on_game_data_ready():
	if not data_ready:
		data_ready = true
		if command_queue.size() > 0:
			_start_queue_processing()

func set_tile_size_and_gap(tile_s: float, tile_gap: float):
	tile_size = tile_s
	gap = tile_gap

func process_command(json_data: Dictionary) -> void:
	if not data_ready:
		command_queue.append(json_data)
		return
	
	if processing_queue:
		command_queue.append(json_data)
		return
	
	_execute_command(json_data)

func _start_queue_processing():
	if command_queue.size() > 0:
		processing_queue = true
		queue_timer.start()
		_process_next_queued_command()

func _process_next_queued_command():
	if command_queue.size() == 0:
		processing_queue = false
		queue_timer.stop()
		return
	
	var command_data = command_queue.pop_front()
	_execute_command(command_data)

func _execute_command(json_data: Dictionary) -> void:
	var command_type = json_data.get("cmd", "")
	var player_id = json_data.get("player_id", -1)

	if (player_id == -1):
		command_failed.emit(command_type, "Invalid player ID")
		return

	match command_type:
		"avance":
			handle_avance(player_id)
		"gauche":
			handle_gauche(player_id)
		"droit":
			handle_droit(player_id)
		"voir":
			handle_voir(player_id)
		"inventaire":
			handle_inventaire(player_id)
		"prend":
			handle_prend(player_id, json_data.get("object", ""))
		"pose":
			handle_pose(player_id, json_data.get("object", ""))
		"incantation":
			handle_incantation(player_id)
		"fork":
			handle_fork(player_id)
		_:
			command_failed.emit(command_type, "Unknown command type")

func handle_avance(player_id: int) -> void:
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		print("Warning: Player ", player_id, " not found in GameData yet")
		command_failed.emit("gauche", "Player data not loaded")
		return

	var current_orientation = player_data.orientation
	player_position_change.emit(player_id, current_orientation, player_data, tile_size + gap)
	command_processed.emit("avance", player_id)

func handle_gauche(player_id: int) -> void:
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		print("Warning: Player ", player_id, " not found in GameData yet")
		command_failed.emit("gauche", "Player data not loaded")
		return

	var current_orientation = player_data.orientation
	var new_orientation = 1 if current_orientation == 4 else current_orientation + 1
	
	player_data.orientation = new_orientation
	
	player_orientation_change.emit(player_id, new_orientation)
	command_processed.emit("gauche", player_id)

func handle_droit(player_id: int) -> void:
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		print("Warning: Player ", player_id, " not found in GameData yet")
		command_failed.emit("droit", "Player data not loaded")
		return

	var current_orientation = player_data.orientation
	var new_orientation = 4 if current_orientation == 1 else current_orientation - 1
	
	player_data.orientation = new_orientation
	
	player_orientation_change.emit(player_id, new_orientation)
	command_processed.emit("droit", player_id)

func handle_voir(player_id: int) -> void:
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		print("Warning: Player ", player_id, " not found in GameData yet")
		command_failed.emit("voir", "Player data not loaded")
		return
	
	# Mock implementation - return what player can see
	var vision_data = ["player", "nourriture", "linemate"]  # Mock data
	print("Player ", player_id, " vision: ", vision_data)
	command_processed.emit("voir", player_id)

func handle_inventaire(player_id: int) -> void:
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		print("Warning: Player ", player_id, " not found in GameData yet")
		command_failed.emit("inventaire", "Player data not loaded")
		return
	
	print("Player ", player_id, " inventory: ", player_data.inventory)
	command_processed.emit("inventaire", player_id)

func handle_prend(player_id: int, object: String) -> void:
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		print("Warning: Player ", player_id, " not found in GameData yet")
		command_failed.emit("prend " + object, player_id)
		return
	
	var current_tile = GameData.tiles[player_data.position]
	var current_tile_resources = current_tile.resources

	if not current_tile_resources[object] > 0.0:
		command_failed.emit("prend", "tile does not contain target object")
		return
	else:
		# Transfer object from tile resources to player inventory
		current_tile.resources[object] -= 1
		player_data.inventory[object] += 1
		
		# Emit signal for object visual transformation
		object_amount_change.emit(player_data.position, object)

	command_processed.emit("prend " + object, player_id)
	GameData.tile_updated.emit(player_data.position.x, player_data.position.y, "RESOURCE_" + object.to_upper())

func handle_pose(player_id: int, object: String) -> void:
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		print("Warning: Player ", player_id, " not found in GameData yet")
		command_failed.emit("prend " + object, player_id)
		return
	
	var current_tile = GameData.tiles[player_data.position]
	var inventory = player_data.get("inventory", {})
	var stored_amount = inventory[object]

	if not stored_amount > 0:
		command_failed.emit("pose", "player doesn't have " + object + " in their inventory")
		return
	else:
		inventory[object] -= 1
		current_tile.resources[object] += 1
		command_processed.emit("pose " + object, player_id)
		object_amount_change.emit(player_data.position, object)
		GameData.tile_updated.emit(player_data.position.x, player_data.position.y, "RESOURCE_" + object.to_upper())
	
	


func handle_incantation(player_id: int) -> void:
	pass

func handle_fork(player_id: int) -> void:
	var player_data = GameData.get_player_data(player_id)

	# Check for existing egg in tile case (maybe not needed)
	var tile_data = GameData.get_tile_data(player_data.position.x, player_data.position.y)
	if not tile_data.eggs.size() == 0:
		command_failed.emit("fork", "tile already has an egg")
		return
	
	var new_egg_id: int
	for egg in GameData.eggs:
		new_egg_id = egg
	new_egg_id += 1

	tile_data.eggs.append(new_egg_id)
	
	var egg_data = {
		"id": new_egg_id,
		"position": player_data.position,
		"status": "laid",
		"parent_id": player_id,
		"team": player_data.team
	}
	egg_laid.emit(egg_data)
	command_processed.emit("fork", player_id)

	# Update possibly displayed panel
	GameData.tile_updated.emit(player_data.position.x, player_data.position.y, "EGG_ADD")
