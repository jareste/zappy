extends WorldEnvironment

"""
WorldEnvironment Controller
Manages environment lighting and background settings
"""

@export var background_color: Color = Color.BLACK
@export var ambient_light_energy: float = 0.3
@export var sky_energy: float = 1.0

func _ready():
	setup_environment()

func setup_environment():
	"""Setup HDRI lighting with solid color background"""
	
	if not environment:
		print("Warning: No environment resource found on WorldEnvironment")
		return
	
	# Set background to solid color instead of sky
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = background_color
	
	# Ensure we still have sky for lighting (should already be set)
	if environment.sky:
		print("HDRI Sky found - using for lighting")
		# Adjust sky energy if needed
		environment.sky_custom_fov = 0.0  # Use full sky
		
		# Optional: Adjust ambient light
		environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
		environment.ambient_light_energy = ambient_light_energy
	else:
		print("Warning: No sky resource found - HDRI lighting unavailable")
	
	# Optional visual enhancements
	if environment.glow_enabled:
		print("Glow enabled for enhanced visuals")
	
	print("Environment setup complete - HDRI lighting active, solid color background")

func set_background_color(color: Color):
	"""Change background color at runtime"""
	background_color = color
	if environment:
		environment.background_color = color

func toggle_sky_background():
	"""Toggle between sky and color background"""
	if not environment:
		return
		
	if environment.background_mode == Environment.BG_SKY:
		environment.background_mode = Environment.BG_COLOR
		environment.background_color = background_color
		print("Switched to color background")
	else:
		environment.background_mode = Environment.BG_SKY
		print("Switched to sky background")
