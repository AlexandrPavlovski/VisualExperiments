#version 460 core

// zeros is replaced at runtime with an actual values
#define particlesCount 0
#define cellsCount 0

layout(std430) buffer;

layout(location = 0) uniform vec2 windowSize;
layout(location = 1) uniform float forceScale;
layout(location = 2) uniform float velocityDamping;
layout(location = 3) uniform float minDistanceToAttractor;
layout(location = 4) uniform float deltaTime;
layout(location = 5) uniform bool isPaused;
layout(location = 6) uniform float particleSize;
layout(location = 7) uniform uint cellSize;

struct Particle
{
	vec2 Pos;
	vec2 Vel;
	vec2 Acc;
	float Pressure;
	float Unused;
};

layout(binding = 0) buffer SSBO
{
	Particle particles[];
};
layout(binding = 6) buffer cells3
{
	uint cellIdsSorted[];
};
layout(binding = 2) buffer objects
{
	vec4 objectIds[];
};


layout(location = 0) out vec3 particleColor;
layout(location = 1) out vec3 gridCellColor;
layout(location = 2) out flat uint isGridCell;

//void screenBounds()
//{
//	vec4 particle = particleData[gl_VertexID];
//	vec2 pos = particle.xy;
//	vec2 vel = particle.zw;
//
//	float halfParticleSize = particleSize / 2;
//	float leftWall = halfParticleSize;
//	float rightWall = windowSize.x - halfParticleSize;
//	float topWall = halfParticleSize;
//	float bottomWall = windowSize.y - halfParticleSize;
//
//	if (pos.x < leftWall)
//	{
//		pos.x = leftWall + leftWall - pos.x;
//		vel.x *= -velocityDamping;
//	}
//	if (pos.y < topWall)
//	{
//		pos.y = topWall + topWall - pos.y;
//		vel.y *= -velocityDamping;
//	}
//	if (pos.x > rightWall)
//	{
//		pos.x = rightWall - (pos.x - rightWall);
//		vel.x *= -velocityDamping;
//	}
//	if (pos.y > bottomWall)
//	{
//		pos.y = bottomWall - (pos.y - bottomWall);
//		vel.y *= -velocityDamping;
//	}
//
//	particleData[gl_VertexID] = vec4(pos, vel);
//}
//
//void ballToBall()
//{
//	vec4 particle = particleData[gl_VertexID];
//	vec2 pos = particle.xy;
//	vec2 vel = particle.zw;
//
//	for (uint j = 0; j < particlesCount; ++j)
//	{
//		if (j == gl_VertexID) continue;
//		vec2 otherPos = particleData[j].xy;
//		vec2 otherVel = particleData[j].zw;
//
//		const vec2 posDiff = pos - otherPos;
//		const float distSquared = dot(posDiff, posDiff);
//	
//		if (distSquared < particleSize * particleSize)
//		{
//			// Move Away
//			const float dist = sqrt(distSquared);
//			const vec2 n = posDiff / dist;
//			const float covered = particleSize - dist;
//			const vec2 moveVec = n * (covered / 2.0f);
//
//			pos += moveVec;
//			otherPos -= moveVec;
//	
//			// Change Velocity Direction
//			const float p = 2 * (dot(n, vel) - dot(n, otherVel)) / 2.0;
//			vel = vel - n * p;
//			otherVel = otherVel + n * p;
//		}
//
//		particleData[j] = vec4(otherPos, otherVel);
//	}
//
//	particleData[gl_VertexID] = vec4(pos, vel);
//}
//
//vec2 getBodyToBodyAcceleration(vec2 b1, vec2 b2)
//{
//	vec2 dir = b2 - b1;
//	
//	float distSquared = max(dot(dir, dir), minDistanceToAttractor);
//	float distSixth = distSquared * distSquared * distSquared;
//	float invDistCube = 1.0f / sqrt(distSixth) * forceScale;
//
//	return dir * invDistCube;
//}
//
//void nBodyGravity()
//{
//	vec4 p = particleData[gl_VertexID];
//	vec2 pos = p.xy;
//	vec2 vel = p.zw;
//
//	vec2 acc = vec2(0.0);
//	for (int i = 0; i < particlesCount; i += 1)
//	{
//		acc += getBodyToBodyAcceleration(pos, particleData[i].xy);
////		acc += getBodyToBodyAcceleration(pos, particleData[i+1].xy);
////		acc += getBodyToBodyAcceleration(pos, particleData[i+2].xy);
////		acc += getBodyToBodyAcceleration(pos, particleData[i+3].xy);
//	}
//
//	vel += acc * deltaTime;
//	vel *= velocityDamping;
//
//	particleData[gl_VertexID] = vec4(pos, vel);
//}
//
//void updatePosition()
//{
//	vec4 particle = particleData[gl_VertexID];
//	vec2 pos = particle.xy;
//	vec2 vel = particle.zw;
//
//	vel += vec2(0.0, 0.001) * deltaTime;
//	pos += vel * deltaTime;
//
//	particleData[gl_VertexID] = vec4(pos, vel);
//}

vec2 collisionResponse(float penetrationDepth, vec2 norm, vec2 relativeVel)
{
	const float k = 1.0;
	const float b = 0.05;

	float acc = k * penetrationDepth - b * dot(norm, relativeVel);

	return acc * norm;
}

vec2 screenBoundsCollision(vec2 pos, vec2 vel)
{
	vec2 acc = vec2(0.0);

	const float halfParticleSize = particleSize / 2;
	const float leftWall = halfParticleSize;
	const float rightWall = windowSize.x - halfParticleSize;
	const float topWall = halfParticleSize;
	const float bottomWall = windowSize.y - halfParticleSize;

	if (pos.x < leftWall)
	{
		acc += collisionResponse(leftWall - pos.x, vec2(1.0, 0.0), vel);
	}
	if (pos.y < topWall)
	{
		acc += collisionResponse(topWall - pos.y, vec2(0.0, 1.0), vel);
	}
	if (pos.x > rightWall)
	{
		acc += collisionResponse(pos.x - rightWall, vec2(-1.0, 0.0), vel);
	}
	if (pos.y > bottomWall)
	{
		acc += collisionResponse(pos.y - bottomWall, vec2(0.0, -1.0), vel);
	}

	return acc;
}

void updateParticle()
{
	float deltaT = deltaTime * 0.04;

	Particle particle = particles[gl_VertexID];

	if (!isPaused)
	{
		const vec2 boundsCollisionAcc = screenBoundsCollision(particle.Pos, particle.Vel);
		const vec2 gravityAcc = vec2(0, 0.1);

		vec2 newVel = (particle.Acc + boundsCollisionAcc + gravityAcc) * deltaT;
		if (dot(newVel, newVel) > 25)
		{
			newVel = normalize(newVel) * 4;
		}

		particle.Vel += newVel;
		particle.Vel *= velocityDamping;
		particle.Pos += particle.Vel * deltaT;

		particle.Acc = vec2(0.0);
		particles[gl_VertexID] = particle;
	}
}

void setParticleColor()
{
	float pressure = particles[gl_VertexID].Pressure / 120.0;
	pressure = float(gl_VertexID) / float(particlesCount);
	// gradient for pressure coloring
	vec3 firstColor =  vec3(0.0, 0.0, 1.0);
	vec3 middleColor = vec3(0.0, 1.0, 0.0);
	vec3 endColor =    vec3(1.0, 0.0, 0.0);

	float h = 0.5;
	vec3 col = mix(mix(firstColor, middleColor, pressure/h), mix(middleColor, endColor, (pressure - h)/(1.0 - h)), step(h, pressure));

	particleColor = vec3(col);
}
//
//void setGridColor()
//{
//	uint cellIndex = gl_VertexID * 4;
////	uint shift = uint(mod(gl_VertexID, 2)) * 16;
////	uint cellId = (cellIds1[cellIndex] >> shift) & 255;
//
//	uvec2 cellIdsPacked = uvec2(cellIdsSorted[cellIndex], cellIdsSorted[cellIndex + 1]);
//	uint hCellId = cellIdsSorted[cellIndex];
//	uint pCellId1 = cellIdsSorted[cellIndex + 1];
//	uint pCellId2 = cellIdsSorted[cellIndex + 2];
//	uint pCellId3 = cellIdsSorted[cellIndex + 3];
//
//	particleData[hCellId + cellsCount].z += 0.7;
//	particleData[pCellId1 + cellsCount].z += 0.7;
//	particleData[pCellId2 + cellsCount].z += 0.7;
//	particleData[pCellId3 + cellsCount].z += 0.7;
//}
//
vec2 worldToScreen(vec2 worldPos)
{
	vec2 windowCenter = windowSize / 2;
	vec2 screenPos = (worldPos - windowCenter) / windowCenter; // convering to clip space [-1; 1]
	screenPos.y = -screenPos.y;

	return screenPos;
}
//
//void drawParticle()
//{
//	updateParticle();
//	setParticleColor();
//	setGridColor();
//
//	vec2 screenPos = worldToScreen(particleData[gl_VertexID].xy);
//	gl_Position = vec4(screenPos, 0.0, 1.0);
//}
//
//void updateGridCells()
//{
//	uint gridWidth = uint(windowSize.x / cellSize) + 2;
//	uint cellId = gl_VertexID - cellsCount;
//
//	vec4 gridCell = particleData[gl_VertexID];
//
//	gridCell.y = ceil(cellId / gridWidth);
//	gridCell.x = cellId - gridCell.y * gridWidth;
//	gridCell.xy *= cellSize;
//	gridCell.xy -= cellSize / 2;
//
//	gridCellColor = vec3(gridCell.z);
//	
////gridCell.xy += 43;
////uint cellIndex = ((gl_VertexID % gridWidth) % 2) + ((gl_VertexID / gridWidth) % 2) * 2;
////if (cellIndex == 0)
////{
////	gridCellColor = vec3(1.0);
////}
//
//	particleData[gl_VertexID].z = mod(gl_VertexID, 2) / 10 + 0.1;
//
//	vec2 screenPos = worldToScreen(gridCell.xy);
//	gl_Position = vec4(screenPos, 0.0, 1.0);
//}
//
//void main()
//{
//	if (gl_VertexID < particlesCount)
//	{
//		drawParticle();
//		isGridCell = 0;
//	}
//	else
//	{
//		updateGridCells();
//		isGridCell = 1;
//	}
//}


void main()
{
	updateParticle();
	setParticleColor();

	vec2 screenPos = worldToScreen(particles[gl_VertexID].Pos);
	gl_Position = vec4(screenPos, 0.0, 1.0);
}
