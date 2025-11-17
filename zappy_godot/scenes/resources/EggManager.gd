extends Node3D

@export var resource_scene: Node3D = null

@export var float_amplitude: float = 0.3
@export var float_frequency: float = 1.0
@export var animation_interval_min: float = 10.0
@export var animation_interval_max: float = 20.0

@onready var animation_player: AnimationPlayer = $AnimationPlayer
var base_position: Vector3
var time_elapsed: float = 0.0
var animation_timer: float = 0.0
var animation_interval = randf_range(animation_interval_min, animation_interval_max)

var hatching_time: float

var ui_ref

func _ready():
	base_position = position
	hatching_time = 600 / MockServer.time_unit_t

func setup_egg_hover_signals(ui_reference, egg_id):
	ui_ref = ui_reference

	set_meta("egg_id", egg_id)

	var egg_area = find_child("Area3D") as Area3D
	
	if egg_area.mouse_entered.is_connected(_on_egg_area_mouse_entered):
		egg_area.mouse_entered.disconnect(_on_egg_area_mouse_entered)
	
	egg_area.mouse_entered.connect(_on_egg_area_mouse_entered.bind(egg_id))
	egg_area.mouse_exited.connect(_on_egg_area_mouse_exited.bind())

func _on_egg_area_mouse_entered(egg_id: int) -> void:
	ui_ref.update_egg_panel(egg_id)

func _on_egg_area_mouse_exited() -> void:
	ui_ref.hide_egg_panel()

func _process(delta: float):
	time_elapsed += delta
	animation_timer += delta
	
	if (time_elapsed >= hatching_time):
		_hatch_egg()
	
	# Floating animation (always running)
	var float_offset = sin(time_elapsed * float_frequency) * float_amplitude
	position = base_position + Vector3(0, float_offset, 0)
	
	# Trigger special animation at intervals
	if animation_timer >= animation_interval:
		_play_resource_animation()
		if animation_timer > 0.0:
			animation_interval = randf_range(10.0, 20.0)
			animation_timer = 0.0

func _play_resource_animation():
	if animation_player and animation_player.has_animation("resource_animation"):
		animation_player.play("resource_animation")
		
		# Wait for animation to finish, then continue floating
		await animation_player.animation_finished
		
func _hatch_egg() -> void:
	var egg_id = _get_egg_id_from_scene()

	if egg_id != -1:
		# Remove from GameData
		if GameData.eggs.has(egg_id):
			var egg_data = GameData.eggs[egg_id]
			var egg_pos = egg_data.position

			#Remove from GameData.eggs
			GameData.eggs.erase(egg_id)

			# Remove from tile data
			var tile_data = GameData.get_tile_data(egg_pos.x, egg_pos.y)
			if tile_data and tile_data.eggs.has(egg_id):
				tile_data.eggs.erase(egg_id)
				GameData.tile_updated.emit(egg_pos.x, egg_pos.y, "EGG_REMOVE")
			
			# Remove from PlayerManager's eggs dictionary
			_notify_player_manager_of_removal(egg_id)

			GameData.tile_updated.emit(tile_data.position.x, tile_data.position.y, "EGG_ADD")

	if ui_ref:
		ui_ref.hide_egg_panel()

	queue_free()

func _get_egg_id_from_scene() -> int:
	"""Get the egg ID that was bound during setup"""
	if has_meta("egg_id"):
		return get_meta("egg_id")

	# Fallback -> search through GameData to find the egg
	for egg_id in GameData.eggs:
		var egg_data = GameData.eggs[egg_id]
		var world_pos = get_parent().get("world_manager").get_world_position(egg_data.position.x, egg_data.position.y) if get_parent().has_method("get_world_position") else Vector3.ZERO
		if global_position.distance_to(world_pos) < 0.1:  # Close enough match
			return egg_id
	
	print("Warning: Could not find egg ID for hatching egg")
	return -1

func _notify_player_manager_of_removal(egg_id: int):
	"""Notify PlayerManager to remove this egg from its tracking"""
	var main_scene = get_tree().current_scene
	if main_scene.has_node("PlayerManager"):
		var player_manager = main_scene.get_node("PlayerManager")
		if player_manager.eggs.has(egg_id):
			player_manager.eggs.erase(egg_id)
