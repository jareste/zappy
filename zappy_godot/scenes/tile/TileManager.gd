extends Area3D

@onready var marker_3d_tl: Marker3D = %Marker3D_TL
@onready var marker_3d_tr: Marker3D = %Marker3D_TR
@onready var marker_3d_br: Marker3D = %Marker3D_BR
@onready var marker_3d_bl: Marker3D = %Marker3D_BL
@onready var marker_3d_tl_2: Marker3D = %Marker3D_TL_2
@onready var marker_3d_tr_2: Marker3D = %Marker3D_TR_2
@onready var marker_3d_br_2: Marker3D = %Marker3D_BR_2
@onready var marker_3d_bl_2: Marker3D = %Marker3D_BL_2

@onready var marker_3d_pl_tl: Marker3D = %Marker3D_PL_TL
@onready var marker_3d_pl_tr: Marker3D = %Marker3D_PL_TR
@onready var marker_3d_pl_bl: Marker3D = %Marker3D_PL_BL
@onready var marker_3d_pl_br: Marker3D = %Marker3D_PL_BR

var resource_positions: Array[Marker3D]
var player_positions: Array[Marker3D]

var available_resource_positions: Array[bool] = [true, true, true, true, true, true, true, true]
var available_player_positions: Array[bool] = [true, true, true, true]

var available_resources: Dictionary = {
		"nourriture": 0,
		"linemate": 0,
		"deraumere": 0,
		"sibur": 0,
		"mendiane": 0,
		"phiras": 0,
		"thystame": 0
	}

var ui_ref
var hovered_tile_x: int
var hovered_tile_y: int

func _ready() -> void:
	setup_resource_grid()
	
	resource_positions = [
		marker_3d_tl,
		marker_3d_tr,
		marker_3d_bl,
		marker_3d_br	,
		marker_3d_tl_2,
		marker_3d_tr_2,
		marker_3d_bl_2,
		marker_3d_br_2
	]

	player_positions = [
		marker_3d_pl_tl,
		marker_3d_pl_tr,
		marker_3d_pl_bl,
		marker_3d_pl_br,
	]


func setup_resource_grid():
	for i in range(4):
		available_resource_positions[i] = true

func setup_tile_hover_signals(tile_data, ui_reference, x: int, y: int):
	ui_ref = ui_reference
	hovered_tile_x = x
	hovered_tile_y = y

	if mouse_entered.is_connected(_on_tile_area_mouse_entered):
		mouse_entered.disconnect(_on_tile_area_mouse_entered)
	
	mouse_entered.connect(_on_tile_area_mouse_entered.bind(tile_data))
	mouse_exited.connect(_on_tile_area_mouse_exited.bind())
	
func _on_tile_area_mouse_entered(tile_data):
	if ui_ref and ui_ref.has_method("update_ui_tile_stats"):
		ui_ref.update_ui_tile_stats(tile_data, hovered_tile_x, hovered_tile_y)

func _on_tile_area_mouse_exited():
	if ui_ref and ui_ref.has_method("hide_tile_info"):
		ui_ref.hide_tile_info()
		
func is_resource_position_available(position_index: int) -> bool:
	if position_index < 0 or position_index >= 8:
		return false
	return available_resource_positions[position_index]

func occupy_resource_position(position_index: int, resource_scene: Node3D, scale_factor: float) -> void:
	if position_index >= 0 and position_index < 8:
		available_resource_positions[position_index] = false
		var marker = resource_positions[position_index] as Marker3D
		marker.add_child(resource_scene)
		resource_scene.global_transform = marker.global_transform

		resource_scene.global_position -= Vector3(0.0, 0.1, 0.0)

		resource_scene.scale = Vector3(scale_factor, scale_factor, scale_factor)


func free_resource_position(position_index: int) -> void:
	if position_index >= 0 and position_index < 8:
		available_resource_positions[position_index] = true

func get_resource_available_position_index() -> int:
	"""Get the next available position index, or -1 if none available"""
	for i in range(8):
		if available_resource_positions[i]:
			return i
	return -1

func get_resource_position_values(position_index: int) -> Vector3:
	"""Get the offset for a specific corner position"""
	if position_index >= 0 and position_index < resource_positions.size():
		return resource_positions[position_index].global_position
	
	print("Warning: Invalid position index: ", position_index)
	return Vector3.ZERO

func is_player_position_available(position_index: int) -> bool:
	if position_index < 0 or position_index >= 4:
		return false
	return available_player_positions[position_index]

func occupy_player_position(position_index: int, player_scene: Node3D, player_id: int) -> void:
	if position_index >= 0 and position_index < 4:
		available_player_positions[position_index] = false
		var marker = player_positions[position_index] as Marker3D
		marker.add_child(player_scene)
		player_scene.global_transform = marker.global_transform

		player_scene.global_position -= Vector3(0.0, 0.21, 0.0)

		var player_orientation = GameData.players[player_id].orientation - 1

		player_scene.rotation += Vector3(0.0, deg_to_rad(90.0) * player_orientation, 0.0)

func free_player_position(position_index: int) -> void:
	if position_index >= 0 and position_index < 4:
		available_player_positions[position_index] = true

func get_player_available_position_index() -> int:
	"""Get the next available position index, or -1 if none available"""
	for i in range(4):
		if available_player_positions[i]:
			return i
	return -1

func get_player_position_values(position_index: int) -> Vector3:
	"""Get the offset for a specific corner position"""
	if position_index >= 0 and position_index < player_positions.size():
		return player_positions[position_index].global_position
	
	print("Warning: Invalid position index: ", position_index)
	return Vector3.ZERO
