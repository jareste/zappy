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
var position_tile: Vector2i

var ui_ref
var type: String

func _ready():
	base_position = position
	
	if animation_player:
		animation_player.stop()

func setup_resource_hover_signals(ui_reference, sent_type: String):
	type = sent_type.to_upper()
	ui_ref = ui_reference

	var resource_area = find_child("Area3D") as Area3D
	
	if resource_area.mouse_entered.is_connected(_on_resource_area_mouse_entered):
		resource_area.mouse_entered.disconnect(_on_resource_area_mouse_entered)
	
	resource_area.mouse_entered.connect(_on_resource_area_mouse_entered.bind())
	resource_area.mouse_exited.connect(_on_resource_area_mouse_exited.bind())

func _on_resource_area_mouse_entered() -> void:
	ui_ref.update_resource_label_display(type, position_tile)

func _on_resource_area_mouse_exited() -> void:
	ui_ref.hide_resource_label_display()

func _process(delta: float):
	time_elapsed += delta
	animation_timer += delta
	
	var float_offset = sin(time_elapsed * float_frequency) * float_amplitude
	position = base_position + Vector3(0, float_offset, 0)
	
	if animation_timer >= animation_interval:
		_play_resource_animation()
		if animation_timer > 0.0:
			animation_interval = randf_range(10.0, 20.0)
			animation_timer = 0.0

func _play_resource_animation():
	if animation_player and animation_player.has_animation("resource_animation"):
		animation_player.play("resource_animation")
		
		await animation_player.animation_finished
