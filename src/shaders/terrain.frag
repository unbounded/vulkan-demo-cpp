#version 450

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 tex;
layout(location = 0) out vec4 f_color;

layout(push_constant) uniform State {
	mat4 mvp;
} state;

const vec3 light = normalize(vec3(1.0, -0.5, 0.7));

const vec3 GROUND_BASE = vec3(20, 83, 10);
const vec3 MOUNTAIN_BASE = vec3(74, 83, 90);
const vec3 LAVA_BASE = vec3(190, 47, 0);
vec4 gen_texture(vec2 position) {
	vec2 c = position - 0.5;
	float dist = length(c);
	dist += cos(atan(c.x, c.y)*10) / 200;
	dist += cos(atan(c.x, c.y)*3 ) / 100;
	if (dist < .17) {
		vec4 col = vec4(LAVA_BASE, 255) / 255;
		col.rg /= (dist / .17);
		return col;
	} else if (dist > 0.27) {
		return vec4(GROUND_BASE, 255) / 255;
	} else {
		return vec4(MOUNTAIN_BASE, 255) / 255;
	}
}

void main() {
    float brightness = dot(normal, light);
    f_color = gen_texture(tex) * brightness;
}