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

var tile_position: Vector2i
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

# Preload resource scenes for performance
var resource_scenes = {
	"linemate": preload("res://scenes/resources/linemateResource.tscn"),
	"deraumere": preload("res://scenes/resources/deraumereResource.tscn"),
	"sibur": preload("res://scenes/resources/siburResource.tscn"),
	"mendiane": preload("res://scenes/resources/mendianeResource.tscn"),
	"phiras": preload("res://scenes/resources/phirasResource.tscn"),
	"thystame": preload("res://scenes/resources/thystameResource.tscn"),
	"nourriture": preload("res://scenes/resources/nourritureResource.tscn")
}

func _ready() -> void:
	CommandProcessor.connect("object_amount_change", _on_object_amount_change)
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

func initialize_tile_position(x: int, y: int):
	tile_position = Vector2i(x, y)

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

func _on_object_amount_change(tile_pos: Vector2i, object: String, change_type: String):
	if tile_position == tile_pos:
		_handle_resource_change(object, tile_pos, change_type)

func _handle_resource_change(object: String, tile_pos: Vector2i, change_type: String):
	var target_child_name = object + "_resource"

	for i in range(resource_positions.size()):
		var marker = resource_positions[i]
		
		for child in marker.get_children():
			if child.name == target_child_name:
				var tile_data = GameData.get_tile_data(tile_pos.x, tile_pos.y)
				if not tile_data:
					print("Warning: Could not find tile data for position ", tile_pos)
					return
				
				var new_quantity = tile_data.resources.get(object, 0)
				
				if change_type == "remove":
					# Handle removing resources (prend command)
					if new_quantity == 0:
						if ui_ref:
							ui_ref.hide_resource_label_display()
						child.queue_free()
						free_resource_position(i)
						available_resources[object] = 0
					else:
						child.get_child(0).quantity = new_quantity
						var scale_factor = 0.5 + (new_quantity - 1) * 0.2
						child.scale = Vector3(scale_factor, scale_factor, scale_factor)
						available_resources[object] = new_quantity
				
				elif change_type == "add":
					# Handle adding resources (pose command)
					child.get_child(0).quantity = new_quantity
					var scale_factor = 0.5 + (new_quantity - 1) * 0.2
					child.scale = Vector3(scale_factor, scale_factor, scale_factor)
					available_resources[object] = new_quantity
				
				return

	# No existing resource scene found - need to create one (only for "add" operations)
	if change_type == "add":
		_place_new_resource_in_tile(tile_pos, object)

func place_initial_resources(tile_data) -> void:
	"""Place all resources for this tile based on tile data - called during world generation"""
	for resource in tile_data.resources:
		var quantity = tile_data.resources[resource]
		if quantity <= 0:
			continue
			
		available_resources[resource] = quantity
		_create_and_place_resource(resource, quantity, tile_data.position)

func _place_new_resource_in_tile(tile_pos: Vector2i, resource: String):
	"""Place a single new resource in the tile - called when placing resources during gameplay"""
	var tile_data = GameData.get_tile_data(tile_pos.x, tile_pos.y)
	if not tile_data:
		print("Warning: Could not find tile data for position ", tile_pos)
		return
		
	var quantity = tile_data.resources.get(resource, 1)
	if quantity <= 0:
		quantity = 1
	
	available_resources[resource] = quantity
	_create_and_place_resource(resource, quantity, tile_pos)

func _create_and_place_resource(resource: String, quantity: int, tile_pos: Vector2i):
	"""Shared logic for creating and placing a resource scene"""
	var scene_resource = resource_scenes.get(resource, resource_scenes["linemate"])
	var resource_scene = scene_resource.instantiate()
	
	var resource_node = resource_scene.get_node(resource) if resource_scene.has_node(resource) else null
	if resource_node:
		resource_node.position_tile = tile_pos
		if resource == "nourriture":
			resource_node.setup_nourriture_hover_signals(ui_ref)
		else:
			resource_node.setup_resource_hover_signals(ui_ref, resource)

	var scale_factor = 0.5 + (quantity - 1) * 0.2
	resource_scene.scale = Vector3(scale_factor, scale_factor, scale_factor)
	resource_scene.get_child(0).quantity = quantity

	var position_index = get_resource_available_position_index()
	if position_index != -1:
		occupy_resource_position(position_index, resource_scene, scale_factor)
	else:
		print("Warning: No available position for resource ", resource, " at tile ", tile_pos)
	
			
