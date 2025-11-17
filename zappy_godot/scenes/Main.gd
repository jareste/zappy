extends Node3D

# Node references
@onready var network_manager = $NetworkManager
@onready var map_root = $MapRoot
@onready var player_root = $PlayerRoot
@onready var egg_root = $EggRoot
@onready var ui = $UI
@onready var camera = $Camera
@onready var camera_controller = $CameraPosition

# Manager components
@onready var world_manager = $WorldManager
@onready var player_manager = $PlayerManager

@export var tile_size: float = 1.0
@export var gap: float = 0.0

func _ready():
	CommandProcessor.set_tile_size_and_gap(tile_size, gap)
	
	test_websocket_connection()

	_setup_managers()
	
	network_manager.connect("line_received", _on_server_line)
	
	_load_test_data()
	
	# Debug signals
	CommandProcessor.command_processed.connect(_on_command_processed)
	CommandProcessor.command_failed.connect(_on_command_failed)
	CommandProcessor.player_orientation_change.connect(_on_player_orientation_changed)

func _setup_managers():
	"""Initialize all manager components"""
	await world_manager.initialize(map_root, ui, tile_size, gap)
	
	player_manager.initialize(player_root, world_manager, egg_root)

	GameData.connect("game_state_updated", _on_game_state_loaded)
	
	print("Zappy GUI initialized successfully")

func _on_game_state_loaded():
	"""Called when game state is loaded - initialize camera position"""
	if GameData.map_size.x > 0 and GameData.map_size.y > 0:
		camera_controller.initialize_camera_for_map(GameData.map_size)

func _load_test_data():
	"""Load test data for demonstration (remove when connecting to real server)"""
	await get_tree().create_timer(1.0).timeout
	
	var file = FileAccess.open("res://data/initial_data/game_3x3.json", FileAccess.READ)
	if file:
		var json_string = file.get_as_text()
		file.close()
		
		var json = JSON.new()
		var parse_result = json.parse(json_string)
		if parse_result == OK:
			GameData.update_game_state(json.data)
			print("Test data loaded successfully")
			
			MockServer.initialize()
		else:
			print("Error parsing JSON: ", json.get_error_message())
	else:
		print("Could not load test data file")

func _on_server_line(line: String):
	"""Handle incoming server messages"""
	print("Zappy server says:", line)
	
	var json = JSON.new()
	var parse_result = json.parse(line)
	if parse_result == OK:
		GameData.update_game_state(json.data)
		return
	
	var tokens = line.split(" ")
	_handle_server_command(tokens)

func _handle_server_command(tokens: Array):
	"""Handle individual server commands. Placeholder until server connection is coded."""
	if tokens.is_empty():
		return
		
	match tokens[0]:
		"msz":
			if tokens.size() >= 3:
				var new_size = Vector2i(int(tokens[1]), int(tokens[2]))
				if GameData.map_size != new_size:
					GameData.map_size = new_size
					print("Map size updated: ", new_size)
		"pnw":
			print("New player detected")
		"ppo":
			print("Player position update")
		_:
			print("Unknown command: ", tokens[0])

func connect_to_server():
	"""Connect to the Zappy server"""
	network_manager.connect_to_server()

func get_game_stats() -> Dictionary:
	"""Get current game statistics for UI"""
	return {
		"map_size": GameData.map_size,
		"player_count": player_manager.get_player_count(),
		"tick": GameData.game_info.get("tick", 0),
		"teams": GameData.teams
	}

func _on_player_orientation_changed(player_id: int, new_orientation: int) -> void:
	pass

# DEBUG 
func _on_command_processed(command_type: String, player_id: int):
	print ("[COMMAND] ", player_id, " -> ", command_type)

func _on_command_failed(command_type: String, error: String):
	print ("Command failed: ", command_type, " - ", error)

# func send_message(message: String):
#     if socket.get_ready_state() == WebSocketPeer.STATE_OPEN:
#         socket.send_text(message)

func test_websocket_connection():
	print("Testing WebSocket connection...")
	print("ZappyWS class available: ", ZappyWS != null)
	
	# if ZappyWS != null:
	if true:
		# var ws_client = ZappyWS.new()
		var ws_client = WebSocketPeer.new()
		print("ZappyWS instance created: ", ws_client != null)
		
		if ws_client != null:
			print("Available methods:")
			# print("  - init: ", ws_client.has_method("init"))
			# print("  - send: ", ws_client.has_method("send"))
			# print("  - recv: ", ws_client.has_method("recv"))
			# print("  - close: ", ws_client.has_method("close"))
			var tls := TLSOptions.client_unsafe()



			var err = ws_client.connect_to_url("wss://127.0.0.1:8674", tls)
			if err != OK:
				print("Unable to connect!!")
				print("#######################################################################################################################################")


			# ws_client.poll()
			# # ws_client.init()
			# print("WebSocket initialized and connecting...")
			
			# # ws_client.send("{\"type\": \"login\", \"key\": \"SOME_KEY\", \"role\": \"observer\"}")

			# var err2 = ws_client.send_text("{\"type\": \"login\", \"key\": \"SOME_KEY\", \"role\": \"observer\"}")
			# if (err2 != OK):
			# 	print("*****************************************************************************************************************")
			

			while ws_client.get_ready_state() == WebSocketPeer.STATE_CONNECTING:
				ws_client.poll()
				await get_tree().process_frame

			if ws_client.get_ready_state() != WebSocketPeer.STATE_OPEN:
				print("Failed to connect!")
				return

			print("Connected! Sending login...")
			ws_client.send_text('{"type":"login","key":"SOME_KEY","role":"observer"}')


			await get_tree().create_timer(2.0).timeout
			
			# print("Sending test message...")
			# ws_client.send("test from godot")

			OS.delay_msec(300)
			# var msg := ws_client.recv()
			# print("Recibido:", msg)

			var message
			while ws_client.get_available_packet_count() > 0:
				var packet = ws_client.get_packet()
				message = packet.get_string_from_utf8()
			# var packet = ws_client.get_packet()
			# var message = packet.get_string_from_utf8()
			if message:
				print("Received: ", message)


			message = null
			ws_client.poll()

			OS.delay_msec(300)
			# var msg := ws_client.recv()
			# print("Recibido:", msg)

			while ws_client.get_available_packet_count() > 0:
				var packet = ws_client.get_packet()
				message = packet.get_string_from_utf8()
				print("message:", message)
			# var packet = ws_client.get_packet()
			# var message = packet.get_string_from_utf8()
			if message:
				print("Received: ", message)


			message = null
			OS.delay_msec(1000)
			ws_client.poll()

			# var msg := ws_client.recv()
			# print("Recibido:", msg)

			while ws_client.get_available_packet_count() > 0:
				var packet = ws_client.get_packet()
				message = packet.get_string_from_utf8()
			# var packet = ws_client.get_packet()
			# var message = packet.get_string_from_utf8()
			if message:
				print("Received: ", message)
			else:
				print("**********************************")

			# print("Attempting to receive messages...")
			# for i in range(10):
			# 	var received = ws_client.recv()
			# 	if received != "":
			# 		print("Received: ", received)
			# 	else:
			# 		print("No message received (attempt ", i + 1, ")")
			# 	await get_tree().create_timer(0.5).timeout
			
			# print("Sending another test message...")
			# ws_client.send("ping")
			
			# await get_tree().create_timer(1.0).timeout
			# var ping_response = ws_client.recv()
			# if ping_response != "":
			# 	print("Ping response: ", ping_response)
			# else:
			# 	print("No ping response received")
			
			print("Closing WebSocket connection...")
			# ws_client.close()
	else:
		print("ZappyWS class not available - extension not loaded")
