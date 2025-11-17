extends Control

var websocket_manager: Node

func _ready():
	print("WebSocket test ready")
	
	websocket_manager = preload("res://scripts/WebSocketManager.gd").new()
	add_child(websocket_manager)
	
	websocket_manager.line_received.connect(_on_message_received)
	websocket_manager.connection_established.connect(_on_connection_established)
	websocket_manager.connection_failed.connect(_on_connection_failed)
	
	await get_tree().create_timer(1.0).timeout
	websocket_manager.connect_to_server()

func _on_message_received(message: String):
	print("Test received: ", message)

func _on_connection_established():
	print("Connection established! Testing commands...")
	await get_tree().create_timer(1.0).timeout
	websocket_manager.send_message("voir")
	
	await get_tree().create_timer(1.0).timeout 
	websocket_manager.send_message("inventaire")
	
	await get_tree().create_timer(1.0).timeout
	websocket_manager.send_message("avance")

func _on_connection_failed():
	print("Connection failed!")

func _input(event):
	if event.is_action_pressed("ui_accept"):
		print("Disconnecting...")
		websocket_manager.disconnect_from_server()
		get_tree().quit()
