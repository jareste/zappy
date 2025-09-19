# WorldManager.gd - Manages the visual representation of the game world (tiles)
extends Node3D

@onready var map_root: Node3D
@export var tile_size = 1.0
@export var gap = 0.1
var tiles = {}


signal world_ready

func _ready():
	# Connect to GameData signals
	GameData.connect("game_state_updated", _on_game_state_updated)
	GameData.connect("tile_updated", _on_tile_updated)

func initialize(map_root_node: Node3D):
	"""Initialize the world manager with the map root node"""
	map_root = map_root_node
	emit_signal("world_ready")

func _on_game_state_updated():
	"""Regenerate the entire world when game state updates"""
	_generate_map()

func _on_tile_updated(x: int, y: int):
	"""Update a specific tile when its data changes"""
	_update_tile_visual(x, y)

func _generate_map():
	"""Generate the visual representation of the map"""
	if not map_root:
		return
		
	# Clear existing tiles
	for child in map_root.get_children():
		child.queue_free()
	tiles.clear()
	
	# Create new tiles
	var spacing = tile_size + gap
	for x in range(GameData.map_size.x):
		for y in range(GameData.map_size.y):
			var tile_scene = preload("res://scenes/tile/tile.tscn").instantiate()
			tile_scene.position = Vector3(x * spacing, 0, y * spacing)
			map_root.add_child(tile_scene)
			tiles[Vector2i(x, y)] = tile_scene
			
			# Update tile visual with current data
			#_update_tile_visual(x, y)
			_set_up_tile(x, y)

func _set_up_tile(x: int,y: int):
	var tile_pos = Vector2i(x, y)
	if not tiles.has(tile_pos):
		return

	var tile_scene = tiles[tile_pos]
	var tile_data = GameData.get_tile_data(x, y)
	if not tile_data:
		return

	for resource in tile_data.resources:
		if tile_data.resources[resource] > 0:
			tile_scene.available_resources[resource] = tile_data.resources[resource]

			var resource_scene := preload("res://scenes/tile/resource.tscn").instantiate()
			var mesh_instance := resource_scene.get_node("MeshInstance3D") as MeshInstance3D
			
			if (resource_scene and mesh_instance):
				var new_material := mesh_instance.material_override.duplicate() as StandardMaterial3D
				var base_color = tile_scene.resource_colors.get(resource)
				var intensity = min(float(tile_scene.available_resources[resource]) / 3.0, 1.0)
				new_material.albedo_color = base_color * intensity + Color.WHITE * (1.0 - intensity)
				mesh_instance.material_override = new_material

				var marker_1 := tile_scene.get_node("Marker3D") as Marker3D
				marker_1.add_child(resource_scene)
				resource_scene.global_transform = marker_1.global_transform
				
				var quantity = tile_scene.available_resources[resource]
				var scale_factor = 1.0 + (quantity - 1) * 0.5  # Increased multiplier for more visible effect
				resource_scene.scale = Vector3(scale_factor, scale_factor, scale_factor)
				
				place_resource_in_tile(tile_scene, resource_scene)
				
				# DEBUG
				print("Resource: ", resource, " Quantity: ", quantity, " Scale: ", scale_factor)

func place_resource_in_tile(tile_scene: Variant, resource_scene: Variant) -> void:
	var x = randi() % 3
	var y = randi() % 3

	while tile_scene.is_position_available(x, y) != true:
		x = randi() % 3
		y = randi() % 3

	tile_scene.occupy_position(x, y)

	var offset: Vector2 = tile_scene.get_position_values(x, y)

	resource_scene.global_position += Vector3(offset.x, 0.0, offset.y)
	print(resource_scene.global_transform)
	
# DEPRECATED
func _update_tile_visual(x: int, y: int):
	"""Update a single tile's visual representation"""
	var tile_pos = Vector2i(x, y)
	if not tiles.has(tile_pos):
		return
		
	var tile_scene = tiles[tile_pos]
	var tile_data = GameData.get_tile_data(x, y)
	
	if not tile_data:
		return
	
	# Update tile color based on resources
	var mesh_instance := tile_scene.get_node("MeshInstance3D") as MeshInstance3D
	if mesh_instance and mesh_instance.material_override:
		var new_material := mesh_instance.material_override.duplicate() as StandardMaterial3D
		
		# Color based on most abundant resource
		var max_resource = 0
		var dominant_resource = ""
		for resource in tile_data.resources:
			if tile_data.resources[resource] > max_resource:
				max_resource = tile_data.resources[resource]
				dominant_resource = resource
		
		# Assign colors based on resource type
		var resource_colors = {
			"nourriture": Color.GREEN,
			"linemate": Color.YELLOW,
			"deraumere": Color.BLUE,
			"sibur": Color.RED,
			"mendiane": Color.PURPLE,
			"phiras": Color.ORANGE,
			"thystame": Color.CYAN
		}
		
		if dominant_resource != "" and max_resource > 0:
			var base_color = resource_colors.get(dominant_resource, Color.GRAY)
			var intensity = min(float(max_resource) / 3.0, 1.0)  # Scale intensity
			new_material.albedo_color = base_color * intensity + Color.WHITE * (1.0 - intensity)
		else:
			# Default color for empty tiles
			new_material.albedo_color = Color(0.3, 0.3, 0.3)
		
		mesh_instance.material_override = new_material

func get_world_position(grid_x: int, grid_y: int) -> Vector3:
	"""Convert grid coordinates to world position"""
	var spacing = tile_size + gap
	return Vector3(grid_x * spacing, 0, grid_y * spacing)

func get_tile_size_with_gap() -> float:
	"""Get the total size of a tile including gap"""
	return tile_size + gap
