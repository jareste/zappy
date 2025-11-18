extends Node

# Integration manager to switch between mock and real server
enum ServerMode { MOCK, REAL }

var current_mode: ServerMode = ServerMode.MOCK
var mock_processor: Node
var server_connection_manager: Node
var active_processor: Node

signal command_processed(command_type: String, player_id: int)
signal command_failed(command_type: String, error: String)
signal server_message_received(message_type: String, data: Dictionary)
signal connection_established
signal connection_failed

func _ready():
	print("Integration Manager ready")
	
	# Initialize processors
	mock_processor = preload("res://scripts/CommandProcessor.gd").new()
	server_connection_manager = preload("res://scripts/ServerConnectionManager.gd").new()
	
	add_child(mock_processor)
	add_child(server_connection_manager)

	# Connect mock processor signals
	mock_processor.connect("command_processed", _on_command_processed)
	mock_processor.connect("command_failed", _on_command_failed)
	
	# Connect real server signals
	server_connection_manager.connect("command_processed", _on_command_processed)
	server_connection_manager.connect("command_failed", _on_command_failed)
	server_connection_manager.connect("server_message_received", _on_server_message_received)
	server_connection_manager.connect("connection_established", _on_connection_established)
	server_connection_manager.connect("connection_failed", _on_connection_failed)
	
	set_mode(ServerMode.MOCK)

func set_mode(mode: ServerMode):
	"""Set the current server mode"""
	current_mode = mode
	
	if mode == ServerMode.MOCK:
		active_processor = mock_processor
		print("Switched to MOCK server mode")
	else:
		active_processor = server_connection_manager
		print("Switched to REAL server mode")

func connect_to_real_server(ip: String, port: int):
	"""Connect to a real Zappy server as an observer"""
	set_mode(ServerMode.REAL)
	server_connection_manager.connect_to_server(ip, port)

func use_mock_server():
	"""Switch to using the mock server"""
	set_mode(ServerMode.MOCK)
	# Disconnect from real server if connected
	if server_connection_manager:
		server_connection_manager.disconnect_from_server()

# Command forwarding methods (only mock server supports commands)
func send_avance(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_avance(player_id)
	else:
		print("Observer mode: Cannot send commands to server")

func send_gauche(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_gauche(player_id)
	else:
		print("Observer mode: Cannot send commands to server")

func send_droite(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_droit(player_id)
	else:
		print("Observer mode: Cannot send commands to server")

func send_voir(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_voir(player_id)
	else:
		print("Observer mode: Cannot send commands to server")

func send_inventaire(player_id: int):
	if current_mode == ServerMode.MOCK:
		print("Mock server doesn't implement inventaire yet")
	else:
		print("Observer mode: Cannot send commands to server")

func send_prend(player_id: int, object_name: String):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_prend(player_id, object_name)
	else:
		print("Observer mode: Cannot send commands to server")

func send_pose(player_id: int, object_name: String):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_pose(player_id, object_name)
	else:
		print("Observer mode: Cannot send commands to server")

# Signal forwarding
func _on_command_processed(command_type: String, player_id: int):
	emit_signal("command_processed", command_type, player_id)

func _on_command_failed(command_type: String, error: String):
	emit_signal("command_failed", command_type, error)

func _on_server_message_received(message_type: String, data: Dictionary):
	emit_signal("server_message_received", message_type, data)

func _on_connection_established():
	print("Successfully connected to real server!")
	emit_signal("connection_established")

func _on_connection_failed():
	print("Failed to connect to real server")
	emit_signal("connection_failed")

func _exit_tree():
	"""Cleanup when shutting down"""
	if server_connection_manager:
		server_connection_manager.disconnect_from_server()
