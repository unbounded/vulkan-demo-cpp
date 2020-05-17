#version 450

layout(location = 0) in vec3 pos0;
layout(location = 1) in vec3 v0;

layout(push_constant) uniform State {
	mat4 mvp;
	float time;
} state;

const float gravity = 1.0;

void main() {
	vec3 pos = pos0 + v0 * state.time;
	pos.y -= gravity * state.time * state.time;
	if (pos.y >= 0) {
		gl_Position = state.mvp * vec4(pos, 1.0);
		gl_PointSize = 10;
	} else {
		gl_Position = vec4(-100, -100, -100, 1.0);
		gl_PointSize = 0;
	}
}