extends Node

func _ready():
	print("Testing ZappyWS with Mock Server...")
	
	await get_tree().create_timer(1.0).timeout
	
	var ws := ZappyWS.new()
	
	if not ws:
		print("ERROR: ZappyWS class not found!")
		return
	
	print("Initializing WebSocket...")
	ws.init()
	
	print("Sending login message...")
	ws.send("{\"type\": \"login\", \"key\": \"SOME_KEY\", \"role\": \"observer\"}")
	
	print("Waiting for server messages...")
	
	for attempt in range(50):
		var msg = ws.recv()
		if msg != "":
			print("Received: ", msg)
			
			var json = JSON.new()
			var parse_result = json.parse(msg)
			if parse_result == OK:
				var data = json.data
				print("  Type: ", data.get("type", "unknown"))
				print("  Data: ", data)
			else:
				print("  Raw message: ", msg)
		
		await get_tree().create_timer(0.5).timeout
	
	print("Closing connection...")
	ws.close()
	print("Mock server test complete!")
