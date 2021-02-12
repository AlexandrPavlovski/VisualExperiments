#version 460 core

// zero is replaced at runtime with an actual value
#define particlesCount 0

layout(location = 0) uniform vec2 windowSize;
layout(location = 1) uniform float forceScale;
layout(location = 2) uniform float velocityDamping;
layout(location = 3) uniform float minDistanceToAttractor;
layout(location = 4) uniform float timeScale;

layout(std430, binding = 0) buffer SSBO
{
	vec4 particleData[];
};

vec2 getBodyToBodyAcceleration(vec2 b1, vec2 b2)
{
	vec2 dir = b2 - b1;
	
	float distSquared = max(dot(dir, dir), minDistanceToAttractor);
	float distSixth = distSquared * distSquared * distSquared;
	float invDistCube = 1.0f / sqrt(distSixth) * forceScale;

	return dir * invDistCube;
}

void main() {
	float deltaT = 0.08 * timeScale;

	vec4 p = particleData[gl_VertexID];
	vec2 pos = p.xy;
	vec2 vel = p.zw;

	vec2 acc = vec2(0.0);
	if (deltaT > 0)
	{
		for (int i = 0; i < particlesCount; i += 4)
		{
			acc += getBodyToBodyAcceleration(pos, particleData[i].xy);
			acc += getBodyToBodyAcceleration(pos, particleData[i+1].xy);
			acc += getBodyToBodyAcceleration(pos, particleData[i+2].xy);
			acc += getBodyToBodyAcceleration(pos, particleData[i+3].xy);
		}
		vel += acc * deltaT;
		vel *= velocityDamping;
		pos += vel * deltaT;
	}
	else // time goes backwards
	{
		pos += vel * deltaT;
		vel *= 1 / velocityDamping;

		for (int i = 0; i <= particlesCount; i += 4)
		{
			acc += getBodyToBodyAcceleration(pos, particleData[i].xy);
			acc += getBodyToBodyAcceleration(pos, particleData[i+1].xy);
			acc += getBodyToBodyAcceleration(pos, particleData[i+2].xy);
			acc += getBodyToBodyAcceleration(pos, particleData[i+3].xy);
		}
		vel += acc * deltaT;
	}

	// teleportation at screen bounds
	pos = mod(mod(pos, windowSize) + vec2(windowSize), vec2(windowSize));

	vec2 normalizedPos = (pos - windowSize / 2) / (windowSize / 2);
	normalizedPos.y = -normalizedPos.y;
	gl_Position = vec4(normalizedPos, 0.0, 1.0);

	p.xy = pos;
	p.zw = vel;
	particleData[gl_VertexID] = p;
}
