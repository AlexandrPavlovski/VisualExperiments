#version 460 core

layout(location = 0) uniform vec2 attractor;
layout(location = 1) uniform vec2 screenSize;
layout(location = 2) uniform float forceScale;
layout(location = 3) uniform float velocityDamping;
layout(location = 4) uniform float minDistanceToAttractor;
layout(location = 5) uniform float timeScale;

layout(std430, binding = 0) buffer SSBO
{
	vec4 particleData[];
};

void main() {
	float deltaT = 16.6666 * timeScale;

	vec4 p = particleData[gl_VertexID];
	vec2 pos = p.xy;
	vec2 vel = p.zw;

	vec2 dir = attractor - pos;
	float distSquared = max(dot(dir, dir), minDistanceToAttractor);
	float F = forceScale / distSquared;

	vel += F * normalize(dir) * deltaT;
	vel *= velocityDamping;
	pos += vel * deltaT;

	// teleportation at screen bounds
	pos = mod(mod(pos, screenSize) + vec2(screenSize), vec2(screenSize));

	vec2 normalizedPos = (pos - screenSize / 2) / (screenSize / 2);
	normalizedPos.y = -normalizedPos.y;
	gl_Position = vec4(normalizedPos, 0.0, 1.0);

	p.xy = pos;
	p.zw = vel;
	particleData[gl_VertexID] = p;
}