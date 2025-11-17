extends Node

signal command_processed(command_type: String, player_id: int)
signal command_failed(command_type: String, error: String)
signal server_message_received(message_type: String, data: Dictionary)

var ws_client: ZappyWS
var connected: bool = false
var authenticated: bool = false
var team_name: String = "default_team"

func _ready():
	print("Real WebSocket CommandProcessor ready")

func connect_to_server(ip: String, port: int, team: String):
	print("Connecting to real server: ", ip, ":", port, " (team: ", team, ")")
	
	ServerConfig.set_server(ip, port)
	ServerConfig.set_team_name(team)
	team_name = team

	ws_client = ZappyWS.new()
	ws_client.init()
	
	await get_tree().create_timer(1.0).timeout

	var welcome = ws_client.recv()
	if welcome != "":
		print("Server welcome: ", welcome)
		connected = true
		_start_authentication()
	else:
		print("Failed to connect to server")
		emit_signal("command_failed", "connection", "Failed to connect to server")

func _start_authentication():
	print("Starting authentication with team: ", team_name)
	
	var login_msg = {
		"type": "login", 
		"team": team_name,
		"role": "player"
	}
	ws_client.send(JSON.stringify(login_msg))
	
	await get_tree().create_timer(0.5).timeout
	var auth_response = ws_client.recv()
	if auth_response != "":
		print("Authentication response: ", auth_response)
		var parsed = JSON.parse_string(auth_response)
		if parsed and parsed.get("type") == "login_success":
			authenticated = true
			print("Authentication successful!")
		else:
			print("Authentication failed: ", auth_response)

func send_command_avance(player_id: int):
	if not _is_ready():
		return
		
	print("Sending AVANCE command to server")
	ws_client.send("avance")
	_wait_for_server_response("avance", player_id)

func send_command_gauche(player_id: int):
	if not _is_ready():
		return
		
	print("Sending GAUCHE command to server")
	ws_client.send("gauche")
	_wait_for_server_response("gauche", player_id)

func send_command_droite(player_id: int):
	if not _is_ready():
		return
		
	print("Sending DROITE command to server")
	ws_client.send("droite")
	_wait_for_server_response("droite", player_id)

func send_command_voir(player_id: int):
	if not _is_ready():
		return
		
	print("Sending VOIR command to server")
	ws_client.send("voir")
	_wait_for_server_response("voir", player_id)

func send_command_prend(player_id: int, object_name: String):
	if not _is_ready():
		return
		
	print("Sending PREND command to server: ", object_name)
	ws_client.send("prend " + object_name)
	_wait_for_server_response("prend", player_id)

func send_command_pose(player_id: int, object_name: String):
	if not _is_ready():
		return
		
	print("Sending POSE command to server: ", object_name)
	ws_client.send("pose " + object_name)
	_wait_for_server_response("pose", player_id)

func _wait_for_server_response(command_type: String, player_id: int):
	await get_tree().create_timer(0.2).timeout
	var response = ws_client.recv()
	
	if response != "":
		print("Server response for ", command_type, ": ", response)
		
		if response == "ok":
			emit_signal("command_processed", command_type, player_id)
		elif response == "ko":
			emit_signal("command_failed", command_type, "Server rejected command")
		else:
			_handle_complex_response(command_type, response, player_id)
	else:
		print("No response from server for command: ", command_type)
		emit_signal("command_failed", command_type, "No server response")

func _handle_complex_response(command_type: String, response: String, player_id: int):
	if command_type == "voir":
		var items = response.split(" ")
		print("Player vision: ", items)
		emit_signal("server_message_received", "vision", {"player_id": player_id, "vision": items})
	elif command_type == "inventaire":
		print("Player inventory: ", response)
		emit_signal("server_message_received", "inventory", {"player_id": player_id, "inventory": response})
	
	emit_signal("command_processed", command_type, player_id)

func _is_ready() -> bool:
	if not connected:
		print("Error: Not connected to server")
		return false
	if not authenticated:
		print("Error: Not authenticated with server")
		return false
	return true

func disconnect_from_server():
	if ws_client:
		ws_client.close()
		connected = false
		authenticated = false
		print("Disconnected from server")
