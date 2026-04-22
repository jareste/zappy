class_name PostProcessingManager
extends Node3D

@export var aberration: float = 0.111 : set = set_aberration
@export var strength: float = 0.1 : set = set_strength
@export var scanline_intensity: float = 0.3 : set = set_scanline_intensity
@export var scanline_thickness: float = 2.593 : set = set_scanline_thickness
@export var noise_intensity: float = 0.1 : set = set_noise_intensity
@export var grain_intensity: float = 0.05 : set = set_grain_intensity
@export var flicker_intensity: float = 0.02 : set = set_flicker_intensity
@export var enabled: bool = true : set = set_enabled

var post_process_material: ShaderMaterial
var mesh_instance: MeshInstance3D
var camera: Camera3D

func _ready():
	# Create the post-processing setup
	setup_post_processing()

func setup_post_processing():
	# Create material with our shader
	post_process_material = ShaderMaterial.new()
	var shader = load("res://materials/shaders/chromatic_aberration.gdshader")
	post_process_material.shader = shader
	
	# Update shader parameters
	update_shader_parameters()
	
	# Find the main camera
	camera = get_viewport().get_camera_3d()
	if not camera:
		push_error("No Camera3D found in the scene!")
		return
	
	# Setup Environment-based post-processing
	if not camera.environment:
		camera.environment = Environment.new()
	
	# Enable some built-in effects that work well with chromatic aberration
	var env = camera.environment
	env.adjustment_enabled = true
	env.adjustment_brightness = 1.0
	env.adjustment_contrast = 1.05
	env.adjustment_saturation = 0.95
	
	# Create a CanvasLayer for our post-processing effect (but make it ignore mouse input)
	var canvas_layer = CanvasLayer.new()
	canvas_layer.name = "PostProcessLayer"
	get_viewport().add_child(canvas_layer)
	
	# Create a ColorRect with our post-processing shader
	var color_rect = ColorRect.new()
	color_rect.material = post_process_material
	color_rect.anchors_preset = Control.PRESET_FULL_RECT
	color_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE  # This prevents input interference!
	color_rect.name = "PostProcessRect"
	
	canvas_layer.add_child(color_rect)
	
	print("Post-processing setup complete!")

func update_shader_parameters():
	if not post_process_material:
		return
		
	post_process_material.set_shader_parameter("aberration", aberration)
	post_process_material.set_shader_parameter("strength", strength)
	post_process_material.set_shader_parameter("scanline_intensity", scanline_intensity)
	post_process_material.set_shader_parameter("scanline_thickness", scanline_thickness)
	post_process_material.set_shader_parameter("noise_intensity", noise_intensity)
	post_process_material.set_shader_parameter("grain_intensity", grain_intensity)
	post_process_material.set_shader_parameter("flicker_intensity", flicker_intensity)

# Setters for real-time parameter updates
func set_aberration(value: float):
	aberration = value
	update_shader_parameters()

func set_strength(value: float):
	strength = value
	update_shader_parameters()

func set_scanline_intensity(value: float):
	scanline_intensity = value
	update_shader_parameters()

func set_scanline_thickness(value: float):
	scanline_thickness = value
	update_shader_parameters()

func set_noise_intensity(value: float):
	noise_intensity = value
	update_shader_parameters()

func set_grain_intensity(value: float):
	grain_intensity = value
	update_shader_parameters()

func set_flicker_intensity(value: float):
	flicker_intensity = value
	update_shader_parameters()

func set_enabled(value: bool):
	enabled = value
	var canvas_layer = get_viewport().get_node_or_null("PostProcessLayer")
	if canvas_layer:
		canvas_layer.visible = enabled

# Method to toggle effect on/off
func toggle_post_processing():
	set_enabled(not enabled)
