extends Node3D

@onready var map_root: Node3D

@export var flat_mode: bool = false

var tile_size: float
var gap: float
var tiles = {}
var ui_reference
var world_ready_emitted = false

# Shared materials for performance optimization
var shared_dark_materials = {}

signal world_ready

func _ready():
	# Connect to GameData signals
	GameData.connect("game_state_updated", _on_game_state_updated)

func initialize(map_root_node: Node3D, ui_node: Control, tile_s: float, tile_gap: float):
	"""Initialize the world manager with the map root node"""
	tile_size = tile_s
	gap = tile_gap
	map_root = map_root_node
	ui_reference = ui_node

func _on_game_state_updated():
	"""Regenerate the entire world when game state updates"""
	print("WorldManager: Game state updated, generating map...")
	print("Map size: ", GameData.map_size)
	print("Total tiles in GameData: ", GameData.tiles.size())
	_generate_map()

func _generate_map():
	"""Generate the visual representation of the map"""
	print("WorldManager: Starting map generation...")
	if not map_root:
		print("WorldManager: No map_root node, aborting generation")
		return

	world_ready_emitted = false
		
	for child in map_root.get_children():
		child.queue_free()
	tiles.clear()

	# Select pattern based on map size
	var selected_pattern = WorldBuilder.select_pattern(flat_mode, GameData.map_size)
	print("WorldManager: Selected pattern: ", selected_pattern)
	
	# Use batch processing for large maps to prevent frame drops
	var total_tiles = GameData.map_size.x * GameData.map_size.y
	print("WorldManager: Total tiles to generate: ", total_tiles)
	if total_tiles > 100:  # For maps larger than 10x10
		_generate_map_batched(selected_pattern)
	else:
		_generate_map_immediate(selected_pattern)

func _generate_map_immediate(selected_pattern):
	"""Generate small maps immediately"""
	var spacing = tile_size + gap
	for x in range(GameData.map_size.x):
		for y in range(GameData.map_size.y):
			_create_and_setup_tile(selected_pattern, x, y, spacing)
	
	_finalize_map_generation()

func _generate_map_batched(selected_pattern):
	"""Generate large maps in batches to prevent frame drops"""
	var spacing = tile_size + gap
	var batch_size = 10  # 10 tiles per frame
	var tiles_to_process = []
	
	# Prepare all tile positions
	for x in range(GameData.map_size.x):
		for y in range(GameData.map_size.y):
			tiles_to_process.push_back(Vector2i(x, y))
	
	# Process in batches
	_process_tile_batch(tiles_to_process, selected_pattern, spacing, batch_size)

func _process_tile_batch(tiles_to_process: Array, selected_pattern, spacing: float, batch_size: int):
	"""Process a batch of tiles"""
	var processed = 0
	while tiles_to_process.size() > 0 and processed < batch_size:
		var pos = tiles_to_process.pop_front()
		_create_and_setup_tile(selected_pattern, pos.x, pos.y, spacing)
		processed += 1
	
	if tiles_to_process.size() > 0:
		# Continue next frame
		await get_tree().process_frame
		_process_tile_batch(tiles_to_process, selected_pattern, spacing, batch_size)
	else:
		_finalize_map_generation()

func _create_and_setup_tile(selected_pattern, x: int, y: int, spacing: float):
	"""Create and setup a single tile"""
	var tile_type = WorldBuilder.get_tile_type_for_position(selected_pattern, x, y, GameData.map_size)
	var tile_scene = _create_tile_from_type(tile_type)
	
	tile_scene.position = Vector3(x * spacing, 0, y * spacing)
	map_root.add_child(tile_scene)
	tiles[Vector2i(x, y)] = tile_scene
	
	_set_up_tile(x, y)

func _finalize_map_generation():
	"""Finalize map generation and emit ready signal"""
	if not world_ready_emitted:
		world_ready_emitted = true
		emit_signal("world_ready")

# Preload tile scenes for performance
var tile_scenes = {
	TileRule.TileType.BASIC: preload("res://scenes/tile/tile_base_scene.tscn"),
	TileRule.TileType.ARCH_1F: preload("res://scenes/tile/tile_arch_01_scene.tscn"),
	TileRule.TileType.ARCH_2F: preload("res://scenes/tile/tile_arch_02_scene.tscn"),
	TileRule.TileType.ARCH_3F: preload("res://scenes/tile/tile_arch_03_scene.tscn")
}

func _create_tile_from_type(tile_type: TileRule.TileType) -> Node3D:
	"""Create a tile scene based on the tile type"""
	var scene_resource = tile_scenes.get(tile_type, tile_scenes[TileRule.TileType.BASIC])
	return scene_resource.instantiate()

func _set_up_tile(x: int,y: int):
	var tile_pos = Vector2i(x, y)
	if not tiles.has(tile_pos):
		return

	var tile_scene = tiles[tile_pos]
	
	var tile_data = GameData.get_tile_data(x, y)
	if not tile_data:
		return

	tile_scene.initialize_tile_position(x, y)
	tile_scene.setup_tile_hover_signals(tile_data, ui_reference, x, y)
	
	_apply_checkerboard_pattern(tile_scene, x, y)
	
	# Let the tile manage its own resource placement
	tile_scene.place_initial_resources(tile_data)

func place_resource_in_tile(tile_scene: Node3D, resource_scene: Node3D, scale_factor: float) -> void:
	var position_index = tile_scene.get_resource_available_position_index()
	
	if position_index == -1:
		print("Warning: No available positions in tile for resource")
		return
	
	tile_scene.occupy_resource_position(position_index, resource_scene, scale_factor)

func get_world_position(grid_x: int, grid_y: int) -> Vector3:
	"""Convert grid coordinates to world position"""
	var spacing = tile_size + gap
	return Vector3(grid_x * spacing, 0, grid_y * spacing)

func get_tile_size_with_gap() -> float:
	"""Get the total size of a tile including gap"""
	return tile_size + gap

func _apply_checkerboard_pattern(tile_scene: Node3D, x: int, y: int):
	"""Apply optimized checkerboard pattern using shared materials"""
	var tile_mesh = _find_tile_mesh(tile_scene)
	if not tile_mesh:
		return

	if not ((x % 2 == 0 and y % 2 != 0) or (x % 2 != 0 and y % 2 == 0)):
		return
	
	var original_material = tile_mesh.material_override
	if not original_material:
		return
	
	var material_key = str(original_material.get_rid())
	if not shared_dark_materials.has(material_key):
		var new_material = original_material.duplicate() as StandardMaterial3D
		new_material.albedo_color = original_material.albedo_color.darkened(0.3)
		shared_dark_materials[material_key] = new_material
	
	tile_mesh.material_override = shared_dark_materials[material_key]

func _find_tile_mesh(tile_scene: Node3D) -> MeshInstance3D:
	"""Find the main tile mesh efficiently"""
	var mesh_paths = [
		"tile_base/basic_tile",
		"tile_base/tile_1f", 
		"tile_base/tile_2f",
		"tile_base/tile_3f"
	]
	
	for path in mesh_paths:
		var mesh = tile_scene.get_node_or_null(path) as MeshInstance3D
		if mesh:
			return mesh
	return null
