#version 460 core

layout(location = 0) uniform vec2 windowSize;
layout(location = 20) uniform vec4 uColor;

layout(std430, binding = 0) buffer SSBO
{
	vec4 particleData[];
};

out vec4 color;

void main() {
//	color = uColor;
	vec2 pCoord = (gl_PointCoord - vec2(0.5)) * 2.0;
	float distSqared = dot(pCoord, pCoord);
	vec3 circle = vec3(smoothstep(1.0, 0.8, distSqared));
	color = vec4(circle, 1.0);
}
