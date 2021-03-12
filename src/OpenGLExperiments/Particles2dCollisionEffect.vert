#version 460 core

// zeros is replaced at runtime with an actual values
#define particlesCount 0
#define particlesCountDoubled 0

layout(std430) buffer;

layout(location = 0) uniform vec2 windowSize;
layout(location = 1) uniform float forceScale;
layout(location = 2) uniform float velocityDamping;
layout(location = 3) uniform float minDistanceToAttractor;
layout(location = 4) uniform float deltaTime;
layout(location = 5) uniform bool isPaused;
layout(location = 6) uniform float particleSize;
layout(location = 7) uniform uint cellSize;

layout(binding = 0) buffer SSBO
{
	vec4 particleData[];
};
layout(binding = 1) buffer cells
{
	uint cellIds[];
};
layout(binding = 2) buffer objects
{
	vec4 objectIds[];
};


layout(location = 0) out vec3 particleColor;
layout(location = 1) out vec3 gridCellColor;
layout(location = 2) out flat uint isGridCell;

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

void updateParticle()
{
	float deltaT = deltaTime * 0.04;

	vec4 particle = particleData[gl_VertexID];
	vec4 particleSecondHalf = particleData[gl_VertexID + particlesCount];
	vec2 pos = particle.xy;
	vec2 vel = particle.zw;
	vec2 acc = particleSecondHalf.yz;

	if (!isPaused)
	{
		vel += acc * deltaT;
		vel *= velocityDamping;
		pos += vel * deltaT;

		particle.xy = pos;
		particle.zw = vel;
		particleData[gl_VertexID] = particle;
		particleData[gl_VertexID + particlesCount].yz = vec2(0.0);
	}
}

void setParticleColor()
{
	float pressure = particleData[gl_VertexID + particlesCount].x / 120.0;
	pressure = float(gl_VertexID) / float(particlesCount);
	// gradient for pressure coloring
	vec3 firstColor =  vec3(0.0, 0.0, 1.0);
	vec3 middleColor = vec3(0.0, 1.0, 0.0);
	vec3 endColor =    vec3(1.0, 0.0, 0.0);

	float h = 0.5;
	vec3 col = mix(mix(firstColor, middleColor, pressure/h), mix(middleColor, endColor, (pressure - h)/(1.0 - h)), step(h, pressure));

	particleColor = vec3(col);
}

void setGridColor()
{
	uint cellIndex = gl_VertexID * 2;
//	uint shift = uint(mod(gl_VertexID, 2)) * 16;
//	uint cellId = (cellIds[cellIndex] >> shift) & 255;

	uvec2 cellIdsPacked = uvec2(cellIds[cellIndex], cellIds[cellIndex + 1]);
	uint hCellId = cellIdsPacked.x & 65535;
	uint pCellId1 = cellIdsPacked.x >> 16;
	uint pCellId2 = cellIdsPacked.y & 65535;
	uint pCellId3 = cellIdsPacked.y >> 16;

	particleData[hCellId + particlesCountDoubled].z += 0.7;
	particleData[pCellId1 + particlesCountDoubled].z += 0.7;
	particleData[pCellId2 + particlesCountDoubled].z += 0.7;
	particleData[pCellId3 + particlesCountDoubled].z += 0.7;
}

vec2 worldToScreen(vec2 worldPos)
{
	vec2 windowCenter = windowSize / 2;
	vec2 screenPos = (worldPos - windowCenter) / windowCenter; // convering to clip space [-1; 1]
	screenPos.y = -screenPos.y;

	return screenPos;
}

void drawParticle()
{
	updateParticle();
	setParticleColor();
	setGridColor();

	vec2 screenPos = worldToScreen(particleData[gl_VertexID].xy);
	gl_Position = vec4(screenPos, 0.0, 1.0);
}

void updateGridCells()
{
	uint gridWidth = uint(windowSize.x / cellSize) + 2;
	uint cellId = gl_VertexID - particlesCountDoubled;

	vec4 gridCell = particleData[gl_VertexID];

	gridCell.y = ceil(cellId / gridWidth);
	gridCell.x = cellId - gridCell.y * gridWidth;
	gridCell.xy *= cellSize;
	gridCell.xy -= cellSize / 2;

	gridCellColor = vec3(gridCell.z);
	particleData[gl_VertexID].z = mod(gl_VertexID, 2) / 10 + 0.1;

	vec2 screenPos = worldToScreen(gridCell.xy);
	gl_Position = vec4(screenPos, 0.0, 1.0);
}

void main()
{
	if (gl_VertexID < particlesCount)
	{
		drawParticle();
		isGridCell = 0;
	}
	else
	{
		updateGridCells();
		isGridCell = 1;
	}
}
