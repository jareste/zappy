extends StaticBody3D

@onready var marker_3d: Marker3D = %Marker3D

var resource_positions : Array[Array] = []
var available_resources: Dictionary = {
		"nourriture": 0,
		"linemate": 0,
		"deraumere": 0,
		"sibur": 0,
		"mendiane": 0,
		"phiras": 0,
		"thystame": 0
	}

var resource_colors = {
			"nourriture": Color.GREEN,
			"linemate": Color.YELLOW,
			"deraumere": Color.BLUE,
			"sibur": Color.RED,
			"mendiane": Color.PURPLE,
			"phiras": Color.ORANGE,
			"thystame": Color.CYAN
		}

func _ready() -> void:
	setup_resource_grid()

func setup_resource_grid():
	resource_positions = []
	for x in range(3):
		resource_positions.append([])
		for y in range(3):
			resource_positions[x].append(true)
	#var random_pos = available_positions[randi() % available_positions.size()]
		
func is_position_available(x: int, y: int) -> bool: return resource_positions[x][y]

func occupy_position(x: int, y: int) -> void: resource_positions[x][y] = false

func get_position_values(raw_x: int, raw_y: int) -> Vector2: return Vector2((raw_x * 0.25) + 0.25, (raw_y * 0.25) + 0.25)
