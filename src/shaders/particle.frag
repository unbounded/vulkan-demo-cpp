#version 450

layout(location = 0) out vec4 f_color;

layout(push_constant) uniform State {
	mat4 mvp;
	float time;
} state;


void main() {
	if (distance(gl_PointCoord, vec2(.5, .5)) < .5) {
		f_color = vec4(236, 112, 34, 1.0) / 255;
	} else {
		f_color = vec4(0.0, 0.0, 0.0, 0.0);
	}
}