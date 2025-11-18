extends Node

signal command_processed(command_type: String, player_id: int)
signal command_failed(command_type: String, error: String)
signal server_message_received(message_type: String, data: Dictionary)
signal connection_established
signal connection_failed

var ws_client: WebSocketPeer
var connected: bool = false
var authenticated: bool = false
var server_ip: String = ""
var server_port: int = 0

func _ready():
	print("Server Connection Manager ready")

func connect_to_server(ip: String, port: int):
	"""Connect to the real Zappy server as an observer"""
	print("Connecting to server as observer: ", ip, ":", port)
	
	server_ip = ip
	server_port = port
	
	_initialize_websocket()

func _initialize_websocket():
	"""Initialize WebSocket connection using Godot's native WebSocketPeer"""
	print("Initializing WebSocket connection...")
	
	ws_client = WebSocketPeer.new()
	
	# Set up TLS options for secure connection
	var tls_options = TLSOptions.client_unsafe()
	var url = "wss://" + server_ip + ":" + str(server_port)
	
	print("Connecting to: ", url)
	var err = ws_client.connect_to_url(url, tls_options)
	
	if err != OK:
		print("Failed to initiate connection, error: ", err)
		emit_signal("connection_failed")
		return
	
	# Wait for connection to establish
	print("Waiting for connection...")
	await _wait_for_connection()

func _wait_for_connection():
	"""Wait for WebSocket connection to establish"""
	var max_attempts = 50  # 5 seconds at 100ms intervals
	var attempts = 0
	
	while ws_client.get_ready_state() == WebSocketPeer.STATE_CONNECTING:
		ws_client.poll()
		await get_tree().create_timer(0.1).timeout
		attempts += 1
		
		if attempts >= max_attempts:
			print("Connection timeout")
			emit_signal("connection_failed")
			return
	
	if ws_client.get_ready_state() == WebSocketPeer.STATE_OPEN:
		print("WebSocket connection established!")
		connected = true
		
		# Give a moment for any welcome message
		await get_tree().create_timer(0.5).timeout
		
		# Check for welcome message
		var welcome = _receive_message()
		if welcome != "":
			print("Server welcome: ", welcome)
		
		_start_authentication()
	else:
		print("Failed to connect to server")
		emit_signal("connection_failed")

func _start_authentication():
	"""Start the authentication process as an observer"""
	print("Starting authentication as observer")
	
	var login_msg = {
		"type": "login",
		"key": "SOME_KEY",
		"role": "observer"
	}
	
	ws_client.send_text(JSON.stringify(login_msg))
	
	await get_tree().create_timer(0.5).timeout
	var auth_response = _receive_message()
	
	if auth_response != "":
		print("Authentication response: ", auth_response)
		_handle_authentication_response(auth_response)
	else:
		print("No authentication response received")
		emit_signal("connection_failed")

func _handle_authentication_response(response: String):
	"""Handle the server's authentication response"""
	var json = JSON.new()
	var parse_result = json.parse(response)
	
	if parse_result == OK:
		var data = json.data
		
		# Check if this is game state data (successful observer authentication)
		if data.has("map") and data.has("players") and data.has("game"):
			authenticated = true
			print("Observer authentication successful! Received initial game state.")
		
			emit_signal("server_message_received", "game_state", data)
			emit_signal("connection_established")
			_start_receiving_game_state()
			return
		
		# Check for explicit success messages (backup)
		elif data.get("type") == "welcome" or data.get("type") == "ok":
			authenticated = true
			print("Observer authentication successful via message!")
			emit_signal("connection_established")
			_start_receiving_game_state()
			return
	
	print("Observer authentication failed: ", response)
	emit_signal("connection_failed")

func _start_receiving_game_state():
	"""Start continuous polling for game state updates"""
	# This will be called periodically to check for server messages
	var timer = Timer.new()
	timer.wait_time = 0.1  # Check for messages every 100ms
	timer.timeout.connect(_poll_server_messages)
	add_child(timer)
	timer.start()

func _poll_server_messages():
	"""Poll for messages from the server"""
	if not _is_ready():
		return
		
	# Poll the WebSocket for new data
	ws_client.poll()
	
	# Check connection state
	var state = ws_client.get_ready_state()
	if state == WebSocketPeer.STATE_CLOSED:
		print("WebSocket connection closed by server")
		connected = false
		authenticated = false
		return
	elif state != WebSocketPeer.STATE_OPEN:
		return
	
	var message = _receive_message()
	if message != "":
		print("Received game state update: ", message)
		_handle_server_message(message)

func _receive_message() -> String:
	"""Receive a message from the WebSocket"""
	if not ws_client:
		return ""
	
	# Poll to update connection state
	ws_client.poll()
	
	# Check if connection is still open
	if ws_client.get_ready_state() != WebSocketPeer.STATE_OPEN:
		return ""
	
	# Read all available packets
	var message = ""
	while ws_client.get_available_packet_count() > 0:
		var packet = ws_client.get_packet()
		message = packet.get_string_from_utf8()
		if message != "":
			break
	
	return message

func _handle_server_message(message: String):
	"""Handle incoming server messages"""
	var json = JSON.new()
	var parse_result = json.parse(message)
	
	if parse_result == OK:
		var data = json.data
		emit_signal("server_message_received", "game_state", data)
	else:
		# Handle non-JSON messages (might be game commands or status updates)
		print("Non-JSON server message: ", message)
		emit_signal("server_message_received", "status", {"message": message})

func _is_ready() -> bool:
	"""Check if connection is ready for sending commands"""
	if not connected:
		print("Error: Not connected to server")
		return false
	if not authenticated:
		print("Error: Not authenticated with server")
		return false
	return true

func disconnect_from_server():
	"""Cleanly disconnect from the server"""
	if ws_client:
		if ws_client.get_ready_state() == WebSocketPeer.STATE_OPEN:
			ws_client.close()
		connected = false
		authenticated = false
		ws_client = null
		print("Disconnected from server")

func _exit_tree():
	"""Cleanup when node is destroyed"""
	disconnect_from_server()
