extends Node

var commands_folder_path: String = "res://data/commands/"
var command_interval: float = 0.2 # seconds
var auto_start: bool = true
var use_realistic_timing: bool = false

var time_unit_t: int = 100
var command_files: Array[String] = []
var current_command_index: int = 0
var command_timer: Timer

signal command_sent(json_data: Dictionary)

func initialize():
	print("Readying MockServer")
	setup_timer()
	load_command_files()

	if auto_start and command_files.size() > 0:
		start_sending_commands()

func set_time_unit_value(unit: int) -> void:
	time_unit_t = unit

func get_command_delay(command_type: String) -> float:
	"""Calculate real delay based on Zappy time units"""
	var time_units: Dictionary = {
		"avance": 7,
		"gauche": 7, 
		"droite": 7,
		"prend": 7,
		"pose": 7,
		"voir": 7,
		"inventaire": 1,
		"incantation": 300,
		"fork": 42
	}

	var delay_in_time_units = time_units.get(command_type, 7)
	return float(delay_in_time_units) / float(time_unit_t)

func setup_timer():
	command_timer = Timer.new()
	command_timer.wait_time = command_interval
	command_timer.timeout.connect(_send_next_command)
	add_child(command_timer)

func load_command_files():
	var dir = DirAccess.open(commands_folder_path)
	if dir:
		dir.list_dir_begin()
		var file_name = dir.get_next()

		while file_name != "":
			if file_name.ends_with(".json"):
				command_files.append(commands_folder_path + file_name)
			file_name = dir.get_next()

		# DEBUG -> sorting won't be necessary after server connection
		command_files.sort()
	else:
		print("Failed to access commands folder: ", commands_folder_path)

func start_sending_commands():
	if command_files.size() > 0:
		command_timer.start()

func stop_sending_commands():
	command_timer.stop()

func _send_next_command():
	if current_command_index >= command_files.size():
		current_command_index = 0
	
	var file_path = command_files[current_command_index]
	var json_data = load_json_file(file_path)

	if json_data:
		var command_type = json_data.get("type", "avance")

		# Send to CommandProcessor
		CommandProcessor.process_command(json_data)

		if use_realistic_timing:
			var next_delay = get_command_delay(command_type)
			command_timer.wait_time = next_delay

		current_command_index += 1

func load_json_file(file_path: String) -> Dictionary:
	var file = FileAccess.open(file_path, FileAccess.READ)
	if file:
		var json_string = file.get_as_text()
		file.close()

		var json = JSON.new()
		var parse_result = json.parse(json_string)

		if parse_result == OK:
			return json.data
		else:
			print("Error parsin JSON file: ", file_path)
			return {}
	else:
		print("Failed to open file: ", file_path)
		return {}
