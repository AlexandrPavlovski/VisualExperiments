#version 460 core

// zeros is replaced at runtime with an actual values
#define particlesCount 0
#define nBody 0

// screen edge behavior - bounce = 1 or stick = 0
#define bounce 0



layout(std430) buffer;

layout(location = 0) uniform vec2 windowSize;
layout(location = 1) uniform float velocityDamping;
layout(location = 2) uniform float deltaTime;
layout(location = 3) uniform bool isPaused;
layout(location = 4) uniform float particleSize;
layout(location = 5) uniform uint cellSize;

struct Particle
{
	vec2 Pos;
	vec2 PosPrev;
	vec2 Acc;
//	float Pressure;
//	float Unused;
};

layout(binding = 0) buffer SSBO
{
	Particle particles[];
};

layout(binding = 13) buffer misc
{
	uint draggedParticleIndex;
};


layout(location = 0) out vec3 particleColor;
layout(location = 1) out vec3 gridCellColor;
layout(location = 2) out flat uint isGridCell;

vec2 getBodyToBodyAcceleration(vec2 b1, vec2 b2)
{
	vec2 dir = b2 - b1;
	
	float distSquared = max(dot(dir, dir), 1.0);
	float distSixth = distSquared * distSquared * distSquared;
	float invDistCube = 1.0f / sqrt(distSixth);

	return dir * invDistCube;
}

void nBodyGravity(inout Particle p)
{
	for (int i = 0; i < particlesCount; i += 1)
	{
		p.Acc += getBodyToBodyAcceleration(p.Pos, particles[i].Pos);
//		acc += getBodyToBodyAcceleration(pos, particleData[i+1].xy);
//		acc += getBodyToBodyAcceleration(pos, particleData[i+2].xy);
//		acc += getBodyToBodyAcceleration(pos, particleData[i+3].xy);
	}
}

vec2 springCollisionResponse(float penetrationDepth, vec2 norm, vec2 relativeVel)
{
	const float k = 1.0;
	const float b = 0.05;

	float acc = k * penetrationDepth - b * dot(norm, relativeVel);

	return acc * norm;
}

vec2 springScreenBoundsCollision(vec2 pos, vec2 vel)
{
	vec2 acc = vec2(0.0);

	const float halfParticleSize = particleSize / 2;
	const float leftWall = halfParticleSize;
	const float rightWall = windowSize.x - halfParticleSize;
	const float topWall = halfParticleSize;
	const float bottomWall = windowSize.y - halfParticleSize;

	if (pos.x < leftWall)
	{
		acc += springCollisionResponse(leftWall - pos.x, vec2(1.0, 0.0), vel);
	}
	if (pos.y < topWall)
	{
		acc += springCollisionResponse(topWall - pos.y, vec2(0.0, 1.0), vel);
	}
	if (pos.x > rightWall)
	{
		acc += springCollisionResponse(pos.x - rightWall, vec2(-1.0, 0.0), vel);
	}
	if (pos.y > bottomWall)
	{
		acc += springCollisionResponse(pos.y - bottomWall, vec2(0.0, -1.0), vel);
	}

	return acc;
}

Particle screenBoundsCollision(Particle particle)
{
	const vec2 pos = particle.Pos;
//	const vec2 posPrev = particle.PosPrev;

	const float halfParticleSize = particleSize / 2;
	const float leftWall = halfParticleSize;// + 400;
	const float rightWall = windowSize.x - halfParticleSize;// - 300;
	const float topWall = halfParticleSize;
	const float bottomWall = windowSize.y - halfParticleSize;

	if (pos.x < leftWall)
	{
#if (bounce == 1)
		particle.Pos.x = leftWall + leftWall - pos.x;
//		particle.PosPrev.x = leftWall - (posPrev.x - leftWall);
#else
		particle.Pos.x = particle.PosPrev.x = leftWall;
#endif
	}
	if (pos.x > rightWall)
	{
#if (bounce == 1)
		particle.Pos.x = rightWall - (pos.x - rightWall);
//		particle.PosPrev.x = rightWall + rightWall - posPrev.x;
#else
		particle.Pos.x = particle.PosPrev.x = rightWall;
#endif
	}
	if (pos.y < topWall)
	{
#if (bounce == 1)
		particle.Pos.y = topWall + topWall - pos.y;
//		particle.PosPrev.y = topWall - (posPrev.y - topWall);
#else
		particle.Pos.y = particle.PosPrev.y = topWall;
#endif
	}
	if (pos.y > bottomWall)
	{
#if (bounce == 1)
		particle.Pos.y = bottomWall - (pos.y - bottomWall);
//		particle.PosPrev.y = bottomWall + bottomWall - posPrev.y;
#else
		particle.Pos.y = particle.PosPrev.y = bottomWall;
#endif
	}

	return particle;
}

void updateParticle()
{
	Particle particle = particles[gl_VertexID];

	if (!isPaused)
	{
		if (draggedParticleIndex - 1 != gl_VertexID)
		{
#if (nBody == 1)
			nBodyGravity(particle);
#else
			const vec2 gravityAcc = vec2(0, 0.005);
			particle.Acc += gravityAcc;
#endif
			vec2 newPos = particle.Pos * 2 - particle.PosPrev + particle.Acc * deltaTime * deltaTime;
			vec2 vel = (newPos - particle.Pos) * velocityDamping;
			particle.PosPrev = newPos - vel;
			particle.Pos = newPos;

			particle.Acc = vec2(0.0);
//			particle.Pressure = 0.0;
		}
	}
	particle = screenBoundsCollision(particle);
	particles[gl_VertexID] = particle;
}

void setParticleColor()
{
	if (draggedParticleIndex - 1 == gl_VertexID)
	{
		particleColor = vec3(1.0, 0.0, 0.0);
		return;
	}

	float pressure = 0.1;//particles[gl_VertexID].Pressure;

//pressure = float(gl_VertexID) / float(particlesCount);
	// gradient for pressure coloring
	vec3 firstColor =  vec3(0.0, 0.0, 1.0);
	vec3 middleColor = vec3(0.0, 1.0, 0.0);
	vec3 endColor =    vec3(1.0, 0.0, 0.0);

	float h = 0.5;
	vec3 col = mix(mix(firstColor, middleColor, pressure/h), mix(middleColor, endColor, (pressure - h)/(1.0 - h)), step(h, pressure));

	particleColor = vec3(col);
}

vec2 worldToScreen(vec2 worldPos)
{
	vec2 windowCenter = windowSize / 2;
	vec2 screenPos = (worldPos - windowCenter) / windowCenter; // convering to clip space [-1; 1]
	screenPos.y = -screenPos.y;

	return screenPos;
}

void main()
{
	setParticleColor();
	updateParticle();

	vec2 screenPos = worldToScreen(particles[gl_VertexID].Pos);
	gl_Position = vec4(screenPos, 0.0, 1.0);
}
