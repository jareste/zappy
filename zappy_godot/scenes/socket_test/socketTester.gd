extends Node

func _ready():
	print("Testing WebSocket extension loading...")
	
	# Debug extension loading
	print("Checking extension file...")
	var ext_file = FileAccess.open("res://ws_lib.gdextension", FileAccess.READ)
	if ext_file:
		print("✓ ws_lib.gdextension found")
		print("Extension file content:")
		var content = ext_file.get_as_text()
		print(content)
		ext_file.close()
	else:
		print("✗ ws_lib.gdextension NOT found")
	
	# Check .so file
	var so_file = FileAccess.open("res://ws_lib/bin/libws.linux.release.x86_64.so", FileAccess.READ)
	if so_file:
		print("✓ .so file found")
		so_file.close()
	else:
		print("✗ .so file NOT found")
	
	# Check all available classes for WebSocket-related ones
	print("Searching for WebSocket classes...")
	var classes = ClassDB.get_class_list()
	var found_classes = []
	
	# First, check for any classes with "Zappy" specifically
	print("Looking for Zappy classes specifically:")
	for cls_name in classes:
		if "Zappy" in str(cls_name):
			print("Found Zappy class: ", cls_name)
			found_classes.append(cls_name)
	
	# Then check for other WebSocket-related classes
	for cls_name in classes:
		if "WS" in str(cls_name) or "WebSocket" in str(cls_name) or "Socket" in str(cls_name):
			if not cls_name in found_classes:  # Avoid duplicates
				found_classes.append(cls_name)
				print("Found WebSocket class: ", cls_name)
	
	if found_classes.is_empty():
		print("ERROR: No WebSocket-related classes found!")
		print("Extension may not be loaded properly.")
		return
	
	# Try each found class
	for cls_name in found_classes:
		print("Trying class: ", cls_name)
		if ClassDB.class_exists(cls_name):
			var instance = ClassDB.instantiate(cls_name)
			if instance:
				print("SUCCESS: Created instance of ", cls_name, ": ", instance)
				
				# Check if it has the methods we need
				if instance.has_method("init"):
					print("  - has init() method ✓")
				if instance.has_method("send"):
					print("  - has send() method ✓")
				if instance.has_method("recv"):
					print("  - has recv() method ✓")
				if instance.has_method("close"):
					print("  - has close() method ✓")
			else:
				print("FAILED to instantiate ", cls_name)
	
	# Try the original ZappyWS name
	print("\nTrying original ZappyWS...")
	if ClassDB.class_exists("ZappyWS"):
		print("ZappyWS class exists!")
		var ws = ClassDB.instantiate("ZappyWS")
		_test_websocket(ws)
	else:
		print("ZappyWS class does not exist")
		
		# Try alternative names
		var alternatives = ["WSBridge", "WebSocketClient", "ZappyWebSocket", "WS"]
		for alt_name in alternatives:
			if ClassDB.class_exists(alt_name):
				print("Found alternative: ", alt_name)
				var ws = ClassDB.instantiate(alt_name)
				_test_websocket(ws)
				break

func _test_websocket(ws):
	if not ws:
		print("No WebSocket instance to test")
		return
		
	print("Testing WebSocket connection with: ", ws)
	
	if not ws.has_method("init"):
		print("ERROR: WebSocket instance doesn't have init() method")
		return
		
	print("Initializing WebSocket...")
	ws.init()
	
	print("Sending login message...")
	ws.send("{\"type\": \"login\", \"key\": \"SOME_KEY\", \"role\": \"observer\"}")
	
	print("Waiting for server response...")
	
	for i in range(100):
		var msg = ws.recv()
		if msg != "":
			print("Received: ", msg)
			break
		await get_tree().create_timer(0.05).timeout
	
	print("Closing connection...")
	ws.close()
	print("WebSocket test complete!")
