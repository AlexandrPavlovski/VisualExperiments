#version 460 core

layout(std430, binding = 0) buffer SSBO
{
	vec2 trianglesData[];
};

void main() {
	vec2 pos = trianglesData[gl_VertexID];
	gl_Position = vec4(pos, 0.0, 1.0);
}
