extends Node

func _ready():
	print("=== Zappy WebSocket Demo ===")
	test_websocket_connection()

func test_websocket_connection():
	print("1. Creating WebSocket client...")
	var ws_client = ZappyWS.new()
	
	print("2. Initializing connection...")
	ws_client.init()  # Hardcoded 127.0.0.1:8674
	
	await get_tree().create_timer(1.0).timeout
	
	print("3. Sending test message...")
	ws_client.send("Hello from Zappy GUI!")
	
	await get_tree().create_timer(0.5).timeout
	
	print("4. Reading response...")
	var response = ws_client.recv()
	if response != "":
		print("✅ Received: ", response)
	else:
		print("⏳ No immediate response")
	
	print("5. Closing connection...")
	ws_client.close()
	
	print("=== Demo Complete ===")
	print("WebSocket integration is ready for your server!")
