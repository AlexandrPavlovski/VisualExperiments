#version 460 core

// zero is replaced at runtime with an actual value
#define particlesCount 0

layout(location = 0) uniform vec2 windowSize;
layout(location = 1) uniform float forceScale;
layout(location = 2) uniform float velocityDamping;
layout(location = 3) uniform float minDistanceToAttractor;
layout(location = 4) uniform float deltaTime;
layout(location = 5) uniform bool isPaused;
layout(location = 6) uniform float particleSize;

layout(std430, binding = 0) buffer SSBO
{
	vec4 particleData[];
};

void screenBounds()
{
	vec4 particle = particleData[gl_VertexID];
	vec2 pos = particle.xy;
	vec2 vel = particle.zw;

	float halfParticleSize = particleSize / 2;
	float leftWall = halfParticleSize;
	float rightWall = windowSize.x - halfParticleSize;
	float topWall = halfParticleSize;
	float bottomWall = windowSize.y - halfParticleSize;

	if (pos.x < leftWall)
	{
		pos.x = leftWall + leftWall - pos.x;
		vel.x *= -velocityDamping;
	}
	if (pos.y < topWall)
	{
		pos.y = topWall + topWall - pos.y;
		vel.y *= -velocityDamping;
	}
	if (pos.x > rightWall)
	{
		pos.x = rightWall - (pos.x - rightWall);
		vel.x *= -velocityDamping;
	}
	if (pos.y > bottomWall)
	{
		pos.y = bottomWall - (pos.y - bottomWall);
		vel.y *= -velocityDamping;
	}

	particleData[gl_VertexID] = vec4(pos, vel);
}

void ballToBall()
{
	vec4 particle = particleData[gl_VertexID];
	vec2 pos = particle.xy;
	vec2 vel = particle.zw;

	for (uint j = 0; j < particlesCount; ++j)
	{
		if (j == gl_VertexID) continue;
		vec2 otherPos = particleData[j].xy;
		vec2 otherVel = particleData[j].zw;

		const vec2 posDiff = pos - otherPos;
		const float distSquared = dot(posDiff, posDiff);
	
		if (distSquared < particleSize * particleSize)
		{
			// Move Away
			const float dist = sqrt(distSquared);
			const vec2 n = posDiff / dist;
			const float covered = particleSize - dist;
			const vec2 moveVec = n * (covered / 2.0f);

			pos += moveVec;
			otherPos -= moveVec;
	
			// Change Velocity Direction
			const float p = 2 * (dot(n, vel) - dot(n, otherVel)) / 2.0;
			vel = vel - n * p;
			otherVel = otherVel + n * p;
		}

		particleData[j] = vec4(otherPos, otherVel);
	}

	particleData[gl_VertexID] = vec4(pos, vel);
}

vec2 getBodyToBodyAcceleration(vec2 b1, vec2 b2)
{
	vec2 dir = b2 - b1;
	
	float distSquared = max(dot(dir, dir), minDistanceToAttractor);
	float distSixth = distSquared * distSquared * distSquared;
	float invDistCube = 1.0f / sqrt(distSixth) * forceScale;

	return dir * invDistCube;
}

void nBodyGravity()
{
	vec4 p = particleData[gl_VertexID];
	vec2 pos = p.xy;
	vec2 vel = p.zw;

	vec2 acc = vec2(0.0);
	for (int i = 0; i < particlesCount; i += 1)
	{
		acc += getBodyToBodyAcceleration(pos, particleData[i].xy);
//		acc += getBodyToBodyAcceleration(pos, particleData[i+1].xy);
//		acc += getBodyToBodyAcceleration(pos, particleData[i+2].xy);
//		acc += getBodyToBodyAcceleration(pos, particleData[i+3].xy);
	}

	vel += acc * deltaTime;
	vel *= velocityDamping;

	particleData[gl_VertexID] = vec4(pos, vel);
}

void updatePosition()
{
	vec4 particle = particleData[gl_VertexID];
	vec2 pos = particle.xy;
	vec2 vel = particle.zw;

	vel += vec2(0.0, 0.001) * deltaTime;
	pos += vel * deltaTime;

	particleData[gl_VertexID] = vec4(pos, vel);
}

void main()
{
//	if (!isPaused) 
//	{
//		screenBounds();
//		ballToBall();
//		nBodyGravity();
//		updatePosition();
//	}

	vec4 particle = particleData[gl_VertexID];
	vec2 pos = particle.xy;
	vec2 vel = particle.zw;

	vec2 windowCenter = windowSize / 2;
	vec2 normalizedPos = (pos - windowCenter) / windowCenter; // convering to clip space [-1; 1]
	normalizedPos.y = -normalizedPos.y;
	gl_Position = vec4(normalizedPos, 0.0, 1.0);
}
