extends Node3D

# Node references
@onready var map_root = $MapRoot
@onready var player_root = $PlayerRoot
@onready var egg_root = $EggRoot
@onready var ui = $UI
@onready var camera = $Camera
@onready var camera_controller = $CameraPosition

# Manager components
@onready var world_manager = $WorldManager
@onready var player_manager = $PlayerManager
@onready var integration_manager = $IntegrationManager

@export var tile_size: float = 1.0
@export var gap: float = 0.0
@export var use_mock_server: bool = true
@export_group("Real Server Settings")
@export var server_ip: String = "127.0.0.1"
@export var server_port: int = 8674
@export var auto_start:bool = false

func _ready():
	CommandProcessor.set_tile_size_and_gap(tile_size, gap)
	
	_setup_managers()
	_setup_server_connection()
	
	# Debug signals
	CommandProcessor.command_processed.connect(_on_command_processed)
	CommandProcessor.command_failed.connect(_on_command_failed)
	CommandProcessor.player_orientation_change.connect(_on_player_orientation_changed)

func _setup_server_connection():
	"""Initialize server connection based on export settings
	
	MOCK MODE: Loads test data → Initialize MockServer → Send commands locally
	REAL MODE: Connect as observer → Wait for server data → Build from server state
	"""
	integration_manager.connect("command_processed", _on_command_processed)
	integration_manager.connect("command_failed", _on_command_failed)
	integration_manager.connect("server_message_received", _on_server_message_received)
	integration_manager.connect("connection_established", _on_real_server_connected)
	integration_manager.connect("connection_failed", _on_real_server_failed)
	
	if use_mock_server:
		print("Using Mock Server")
		integration_manager.use_mock_server()
		_load_test_data()
	else:
		print("Connecting to Real Server as Observer: ", server_ip, ":", server_port)
		integration_manager.connect_to_real_server(server_ip, server_port)
		if auto_start:
			print("**********************************************AUTOSTART**************************************")
			_start_server_time_api()

func _start_server_time_api():
	"""Start the server's time API to begin the game"""
	var script_path = ProjectSettings.globalize_path("/home/hmunoz-g/42-OuterCore/zappy/server/run.sh")
	var output = []
	var exit_code = OS.execute("bash", [script_path], output, true, true)
	
	if exit_code == 0:
		print("Game time started successfully")
		for line in output:
			print("Server output: ", line)
	else:
		print("Failed to start game time. Exit code: ", exit_code)
		for line in output:
			print("Error output: ", line)

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
	"""Load test data for mock server mode only"""
	print("Loading test data for mock server...")
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
			
			# Initialize MockServer only in mock mode
			MockServer.initialize()
		else:
			print("Error parsing JSON: ", json.get_error_message())
	else:
		print("Could not load test data file")

func _on_real_server_connected():
	"""Called when successfully connected to real server"""
	print("Successfully connected to real server as observer")
	print("Waiting for initial game state from server...")

func _on_real_server_failed():
	"""Called when real server connection failed"""
	print("Failed to connect to real server")
	print("Consider switching to mock mode for development")

func _on_server_message_received(message_type: String, data: Dictionary):
	"""Handle messages from real server"""
	print("Server message received: ", message_type)
	
	match message_type:
		"game_state":
			GameData.update_game_state(data)
			print("Game world built from server data")
		"status":
			print("Server status: ", data.get("message", "Unknown status"))
		_:
			print("Unknown message type: ", message_type, " - ", data)

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
