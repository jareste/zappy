class_name ChromaticAberrationEffect
extends CompositorEffect

# Shader parameters
var aberration: float = 0.111 : set = set_aberration
var strength: float = 0.1 : set = set_strength
var scanline_intensity: float = 0.3 : set = set_scanline_intensity
var scanline_thickness: float = 2.593 : set = set_scanline_thickness
var noise_intensity: float = 0.1 : set = set_noise_intensity
var grain_intensity: float = 0.05 : set = set_grain_intensity
var flicker_intensity: float = 0.02 : set = set_flicker_intensity

var material: ShaderMaterial
var quad_mesh: QuadMesh

func _init():
	effect_callback_type = CompositorEffect.EFFECT_CALLBACK_TYPE_POST_TRANSPARENT
	needs_motion_vectors = false
	needs_separate_specular = false
	
	# Load the spatial shader
	var shader = load("res://materials/shaders/chromatic_aberration_spatial.gdshader")
	material = ShaderMaterial.new()
	material.shader = shader
	
	# Create quad mesh for full-screen effect
	quad_mesh = QuadMesh.new()
	quad_mesh.size = Vector2(2.0, 2.0)
	
	# Set initial shader parameters
	_update_shader_params()

func _update_shader_params():
	if not material:
		return
		
	material.set_shader_parameter("aberration", aberration)
	material.set_shader_parameter("strength", strength)
	material.set_shader_parameter("scanline_intensity", scanline_intensity)
	material.set_shader_parameter("scanline_thickness", scanline_thickness)
	material.set_shader_parameter("noise_intensity", noise_intensity)
	material.set_shader_parameter("grain_intensity", grain_intensity)
	material.set_shader_parameter("flicker_intensity", flicker_intensity)

func _render_callback(_effect_callback_type: int, _render_data: RenderData):
	# For now, we'll implement this using a simpler approach
	# The CompositorEffect system is quite complex and requires deep RenderingDevice knowledge
	# Let's use a hybrid approach with SubViewport instead
	pass

# Setters for real-time parameter updates
func set_aberration(value: float):
	aberration = value
	_update_shader_params()

func set_strength(value: float):
	strength = value
	_update_shader_params()

func set_scanline_intensity(value: float):
	scanline_intensity = value
	_update_shader_params()

func set_scanline_thickness(value: float):
	scanline_thickness = value
	_update_shader_params()

func set_noise_intensity(value: float):
	noise_intensity = value
	_update_shader_params()

func set_grain_intensity(value: float):
	grain_intensity = value
	_update_shader_params()

func set_flicker_intensity(value: float):
	flicker_intensity = value
	_update_shader_params()
