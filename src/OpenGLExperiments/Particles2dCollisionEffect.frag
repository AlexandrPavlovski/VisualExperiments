#version 460 core

// 0 is replaced at runtime with an actual value
#define particlesCount 0

layout(location = 0) uniform vec2 windowSize;
layout(location = 20) uniform vec4 uColor;

layout(location = 0) in vec3 particleColor;
layout(location = 1) in vec3 gridCellColor;

out vec4 color;

void main() {
//	color = uColor;
	vec2 pCoord = (gl_PointCoord - vec2(0.5)) * 2.0;
	float distSqared = dot(pCoord, pCoord);
	float circle = smoothstep(1.0, 0.9, distSqared);

	color = vec4(particleColor, circle);
}
