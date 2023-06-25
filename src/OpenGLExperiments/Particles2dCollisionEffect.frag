#version 460 core

layout(location = 0) in vec3 particleColor;

out vec4 color;

void main() {
	vec2 pCoord = (gl_PointCoord - vec2(0.5)) * 2.0;
	float distSqared = dot(pCoord, pCoord);
	float circle = smoothstep(1.0, 0.95, distSqared);

	color = vec4(particleColor, circle);
}
