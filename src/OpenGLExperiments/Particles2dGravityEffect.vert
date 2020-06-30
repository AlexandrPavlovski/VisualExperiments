#version 460 core

layout(location = 0) uniform vec2 attractor;
layout(location = 1) uniform vec2 windowSize;
layout(location = 2) uniform float forceScale;
layout(location = 3) uniform float velocityDamping;
layout(location = 4) uniform float minDistanceToAttractor;
layout(location = 5) uniform float timeScale;

layout(std430, binding = 0) buffer SSBO
{
	vec4 particleData[];
};

vec2 getBodyToBodyAcceleration1(vec2 b1, vec2 b2)
{
	vec2 dir = b2 - b1;
	float distSquared = max(dot(dir, dir), minDistanceToAttractor);
	float F = forceScale / distSquared;

	return F * normalize(dir);
}

vec2 getBodyToBodyAcceleration2(vec2 b1, vec2 b2)
{
	vec2 dir = b2 - b1;
	
	float distSquared = dot(dir, dir) + minDistanceToAttractor;
	float distSixth = distSquared * distSquared * distSquared;
	float invDistCube = 1.0f / sqrt(distSixth) * forceScale;

	return dir * invDistCube;
}

void main() {
	float deltaT = 16.6666 * timeScale;

	vec4 p = particleData[gl_VertexID];
	vec2 pos = p.xy;
	vec2 vel = p.zw;

	vec2 acc = getBodyToBodyAcceleration1(pos, attractor);

	vel += acc * deltaT;
	vel *= velocityDamping;
	pos += vel * deltaT;

	// teleportation at screen bounds
	pos = mod(mod(pos, windowSize) + vec2(windowSize), vec2(windowSize));

	vec2 normalizedPos = (pos - windowSize / 2) / (windowSize / 2);
	normalizedPos.y = -normalizedPos.y;
	gl_Position = vec4(normalizedPos, 0.0, 1.0);

	p.xy = pos;
	p.zw = vel;
	particleData[gl_VertexID] = p;
}