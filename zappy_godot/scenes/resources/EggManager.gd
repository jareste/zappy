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

var ui_ref

func _ready():
	base_position = position

func setup_egg_hover_signals(ui_reference, egg_id):
	ui_ref = ui_reference

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
