#version 460 core

// 0 is replaced at runtime with an actual value
#define particlesCount 0

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
	float circle = smoothstep(0.3, 0.1, distSqared);

	color = vec4(circle, circle, circle, 1.0);
}
