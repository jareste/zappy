shader_type spatial;
render_mode unshaded, fog_disabled;

uniform sampler2D screen_tex : source_color, hint_screen_texture;
uniform vec2 aberration_direction = vec2(1.0, 0.0);
uniform float strength: hint_range(0.0, 0.1, 0.001) = 0.01;

void vertex() {
    POSITION = vec4(VERTEX.xy, 1.0, 1.0);
}

void fragment() {
    vec2 suv = SCREEN_UV;
    vec2 aberration_vec = aberration_direction * strength;
    
    vec4 red = texture(screen_tex, suv - aberration_vec);
    vec4 green = texture(screen_tex, suv + aberration_vec);
    vec4 blue = texture(screen_tex, suv);
    
    ALBEDO = vec3(red.r, green.g, blue.b);
}