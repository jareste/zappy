extends Node

var ws_client: ZappyWS
var connected := false
var authenticated := false

signal line_received(line: String)
signal connection_established
signal connection_failed

func _ready():
	print("NetworkManager ready, ZappyWS class available: ", ZappyWS != null)

func connect_to_server(address: String = "ws://localhost:8674"):
	print("Connecting to server: ", address)
	print("*********************************************************************************************************************************************************************")
	
	ws_client = ZappyWS.new()
	var result = ws_client.init(address)
	
	if result == 0:
		print("WebSocket connection initiated")
		connected = true
		_start_authentication()
	else:
		print("Failed to connect to WebSocket server")
		emit_signal("connection_failed")

func _start_authentication():
	print("Starting authentication process")

func send_message(message: String):
	if connected and ws_client:
		var result = ws_client.send(message)
		print("Sent message: ", message, " (result: ", result, ")")

func _process(delta):
	if not connected or not ws_client:
		return
	
	var message = ws_client.recv()
	if message != "":
		print("Received message: ", message)
		emit_signal("line_received", message)
		
		if not authenticated:
			_handle_authentication(message)

func _handle_authentication(message: String):
	if message == "BIENVENUE":
		print("Received welcome, sending team name")
		send_message("test_team")
	elif message.is_valid_int():
		print("Received client number: ", message)
	elif message.find(" ") > 0 and message.split(" ").size() == 2:
		var parts = message.split(" ")
		if parts[0].is_valid_int() and parts[1].is_valid_int():
			print("Received world dimensions: ", message)
			authenticated = true
			emit_signal("connection_established")

func disconnect_from_server():
	if ws_client:
		ws_client.close()
		connected = false
		authenticated = false
		ws_client = null
		print("Disconnected from server")
