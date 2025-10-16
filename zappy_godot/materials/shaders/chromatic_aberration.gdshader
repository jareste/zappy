shader_type canvas_item;
uniform sampler2D screen_tex: hint_screen_texture, filter_linear;
uniform float aberration: hint_range(0.0, 1.5, 0.001) = 0.5;
uniform float strength: hint_range(0.0, 0.1, 0.001) = 0.05;

// CRT Effects
uniform float scanline_intensity : hint_range(0.0, 1.0) = 0.3;
uniform float scanline_thickness : hint_range(0.1, 5.0) = 1.0;
uniform float noise_intensity : hint_range(0.0, 1.0) = 0.1;
uniform float grain_intensity : hint_range(0.0, 1.0) = 0.05;
uniform float flicker_intensity : hint_range(0.0, 1.0) = 0.02;

// Noise function for film grain and static
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Noise with time for animated static
float noise(vec2 st, float time) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = random(i + time);
    float b = random(i + vec2(1.0, 0.0) + time);
    float c = random(i + vec2(0.0, 1.0) + time);
    float d = random(i + vec2(1.0, 1.0) + time);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void fragment() {
	vec2 suv = SCREEN_UV;
	float aspect_ratio = SCREEN_PIXEL_SIZE.y / SCREEN_PIXEL_SIZE.x;
	vec2 scaled_suv = (suv - vec2(0.0, 0.5)) / vec2(1.0, aspect_ratio) + vec2(0, 0.5);
	vec2 offset = normalize(scaled_suv) * strength;
	vec2 aberration_vec = offset * aberration * strength;

	vec4 red = texture(screen_tex, suv - aberration_vec);
	vec4 green = texture(screen_tex, suv + aberration_vec);
	vec4 blue = texture(screen_tex, suv);

	COLOR = vec4(red.r, green.g, blue.b, 1.0);
}