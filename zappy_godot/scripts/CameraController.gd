extends Node3D

@onready var camera_rotation_x: Node3D = %CameraRotationX
@onready var camera_zoom_pivot: Node3D = %CameraZoomPivot
@onready var camera: Camera3D = %Camera
@onready var world_manager: Node3D = %WorldManager

@export var move_speed = 0.6
@export var zoom_speed = 6.0
@export var rotation_speed = 2.0
@export var lerp_speed = 0.05
@export var viewport_margin_ratio = 0.1
@export var isometric_angle = -5.0
@export var default_y_rotation = 45.0
@export var use_orthogonal = true  # Toggle between ortho and perspective

# Orthogonal specific settings
@export var ortho_size_base = 10.0

var move_target: Vector3
var zoom_target: float
var rotation_target: float
var is_initialized = false
var movement_bounds = Rect2()
var initial_ortho_size: float

var q_pressed_last_frame = false
var e_pressed_last_frame = false

func _ready() -> void:
	rotation_target = rotation.y
	
	if use_orthogonal:
		camera.projection = Camera3D.PROJECTION_ORTHOGONAL
		camera.size = ortho_size_base
		camera.near = 0.05
		camera.far = 200.0
	else:
		camera.projection = Camera3D.PROJECTION_PERSPECTIVE

func initialize_camera_for_map(map_size: Vector2i):
	var map_center = Vector3(
		(map_size.x - 1) * 0.5,
		0,
		(map_size.y - 1) * 0.5
	)
	
	camera_rotation_x.rotation.x = deg_to_rad(isometric_angle)
	rotation.y = deg_to_rad(default_y_rotation)
	rotation_target = rotation.y
	
	position = map_center
	move_target = position
	
	if use_orthogonal:
		var optimal_size = _calculate_optimal_ortho_size(map_size)
		camera.size = optimal_size
		zoom_target = optimal_size
		initial_ortho_size = optimal_size
		camera_zoom_pivot.position.z = 25.0
	else:
		var optimal_zoom = _calculate_optimal_ortho_size(map_size)
		camera_zoom_pivot.position.z = optimal_zoom
		zoom_target = optimal_zoom
	
	var padding = max(map_size.x, map_size.y) * 0.5
	movement_bounds = Rect2(
		-padding, -padding,
		map_size.x + padding * 2, map_size.y + padding * 2
	)
	
	is_initialized = true

func _calculate_optimal_ortho_size(map_size: Vector2i) -> float:
	var max_dimension = max(map_size.x, map_size.y)
	var base_size = max_dimension + 5.0
	var final_size = base_size * (0.8 + viewport_margin_ratio * 5.0)
	return final_size

func _handle_zoom(_delta: float) -> void:
	if use_orthogonal:
		var min_size = 2.0
		var max_size = initial_ortho_size if initial_ortho_size > 0 else 50.0
		zoom_target = clamp(zoom_target, min_size, max_size)
		camera.size = lerp(camera.size, zoom_target, lerp_speed)
	else:
		var min_zoom = 2.0
		var max_zoom = max(20.0, zoom_target * 2.0)
		zoom_target = clamp(zoom_target, min_zoom, max_zoom)
		camera_zoom_pivot.position.z = lerp(camera_zoom_pivot.position.z, zoom_target, lerp_speed)

func _input(event):
	if event is InputEventMouseButton:
		match event.button_index:
			MOUSE_BUTTON_WHEEL_UP:
				if use_orthogonal:
					zoom_target -= zoom_speed * 0.2
				else:
					zoom_target -= zoom_speed * 0.1
			MOUSE_BUTTON_WHEEL_DOWN:
				if use_orthogonal:
					zoom_target += zoom_speed * 0.2
				else:
					zoom_target += zoom_speed * 0.1

func _process(delta: float) -> void:
	if not is_initialized:
		return
		
	_handle_movement(delta)
	_handle_zoom(delta)
	_handle_rotation(delta)

func _handle_movement(_delta: float) -> void:
	var input_direction = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	
	if Input.is_action_pressed("ui_left") or Input.is_key_pressed(KEY_A):
		input_direction.x -= 1.0
	if Input.is_action_pressed("ui_right") or Input.is_key_pressed(KEY_D):
		input_direction.x += 1.0
	if Input.is_action_pressed("ui_up") or Input.is_key_pressed(KEY_W):
		input_direction.y -= 1.0
	if Input.is_action_pressed("ui_down") or Input.is_key_pressed(KEY_S):
		input_direction.y += 1.0
	
	var movement_local = Vector3(input_direction.x, 0, input_direction.y) * move_speed
	var rotated_movement = movement_local.rotated(Vector3.UP, rotation.y)
	
	move_target += rotated_movement
	
	if movement_bounds != Rect2():
		move_target.x = clamp(move_target.x, movement_bounds.position.x, movement_bounds.position.x + movement_bounds.size.x)
		move_target.z = clamp(move_target.z, movement_bounds.position.y, movement_bounds.position.y + movement_bounds.size.y)
	
	position = lerp(position, move_target, lerp_speed)

func _handle_rotation(_delta: float) -> void:
	var q_pressed = Input.is_key_pressed(KEY_Q)
	var e_pressed = Input.is_key_pressed(KEY_E)
	
	if q_pressed and not q_pressed_last_frame:
		rotation_target -= PI/2
	if e_pressed and not e_pressed_last_frame:
		rotation_target += PI/2
	
	q_pressed_last_frame = q_pressed
	e_pressed_last_frame = e_pressed
	
	rotation.y = lerp_angle(rotation.y, rotation_target, lerp_speed * 2.0)

func reset_camera_to_overview():
	if is_initialized and GameData.map_size.x > 0:
		initialize_camera_for_map(GameData.map_size)

func focus_on_position(world_pos: Vector3, zoom_level: float = -1):
	move_target = world_pos
	if zoom_level > 0:
		zoom_target = zoom_level

func toggle_projection():
	use_orthogonal = !use_orthogonal
	if use_orthogonal:
		camera.projection = Camera3D.PROJECTION_ORTHOGONAL
		camera.size = zoom_target if zoom_target > 0 else ortho_size_base
	else:
		camera.projection = Camera3D.PROJECTION_PERSPECTIVE
		camera.fov = 75.0
