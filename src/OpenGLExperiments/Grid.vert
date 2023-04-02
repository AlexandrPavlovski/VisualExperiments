#version 460 core

vec2 trianglesData[3] = vec2[3]
(
	vec2(-1.0, 1.0),
	vec2(3.0, 1.0),
	vec2(-1.0, -3.0)
);

void main() {
	vec2 pos = trianglesData[gl_VertexID];
	gl_Position = vec4(pos, 0.0, 1.0);
}
