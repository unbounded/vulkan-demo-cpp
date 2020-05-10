#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex;
    
layout(location = 0) out vec3 normal_out;
layout(location = 1) out vec2 tex_out;

layout(push_constant) uniform State {
	mat4 mvp;
} state;

void main() {
	gl_Position = state.mvp * vec4(pos, 1.0);
	normal_out = normal;
	tex_out = tex;
}