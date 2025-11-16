extends Node

var tcp := StreamPeerTCP.new()
var connected := false

signal line_received(line: String)

func connect_to_server():
	var err = tcp.connect_to_host("127.0.0.1", 4242)
	if err == OK:
		print("Connected to Zappy server")
		connected = true
		tcp.put_utf8_string("GRAPHIC\n")

func _process(delta):
	if not connected:
		return

	while tcp.get_available_bytes() > 0:
		var line = tcp.get_utf8_string(tcp.get_available_bytes())
		for l in line.split("\n"):
			if l.strip_edges() != "":
				emit_signal("line_received", l.strip_edges())
