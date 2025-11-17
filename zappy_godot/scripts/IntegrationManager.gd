extends Node

# Integration manager to switch between mock and real server
enum ServerMode { MOCK, REAL }

var current_mode: ServerMode = ServerMode.MOCK
var mock_processor: Node
var real_processor: Node
var active_processor: Node

signal command_processed(command_type: String, player_id: int)
signal command_failed(command_type: String, error: String)
signal server_message_received(message_type: String, data: Dictionary)

func _ready():
	print("Integration Manager ready")
	
	mock_processor = preload("res://scripts/CommandProcessor.gd").new()
	real_processor = preload("res://scripts/RealServerCommandProcessor.gd").new()
	
	add_child(mock_processor)
	add_child(real_processor)

	mock_processor.connect("command_processed", _on_command_processed)
	mock_processor.connect("command_failed", _on_command_failed)
	
	real_processor.connect("command_processed", _on_command_processed)
	real_processor.connect("command_failed", _on_command_failed)
	real_processor.connect("server_message_received", _on_server_message_received)
	
	set_mode(ServerMode.MOCK)

func set_mode(mode: ServerMode):
	current_mode = mode
	
	if mode == ServerMode.MOCK:
		active_processor = mock_processor
		print("Switched to MOCK server mode")
	else:
		active_processor = real_processor
		print("Switched to REAL server mode")

func connect_to_real_server(ip: String, port: int, team: String):
	set_mode(ServerMode.REAL)
	real_processor.connect_to_server(ip, port, team)

func use_mock_server():
	set_mode(ServerMode.MOCK)

func send_avance(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_avance(player_id)
	else:
		real_processor.send_command_avance(player_id)

func send_gauche(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_gauche(player_id)
	else:
		real_processor.send_command_gauche(player_id)

func send_droite(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_droit(player_id)
	else:
		real_processor.send_command_droite(player_id)

func send_voir(player_id: int):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_voir(player_id)
	else:
		real_processor.send_command_voir(player_id)

func send_prend(player_id: int, object_name: String):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_prend(player_id, object_name)
	else:
		real_processor.send_command_prend(player_id, object_name)

func send_pose(player_id: int, object_name: String):
	if current_mode == ServerMode.MOCK:
		mock_processor.handle_pose(player_id, object_name)
	else:
		real_processor.send_command_pose(player_id, object_name)

func _on_command_processed(command_type: String, player_id: int):
	emit_signal("command_processed", command_type, player_id)

func _on_command_failed(command_type: String, error: String):
	emit_signal("command_failed", command_type, error)

func _on_server_message_received(message_type: String, data: Dictionary):
	emit_signal("server_message_received", message_type, data)
