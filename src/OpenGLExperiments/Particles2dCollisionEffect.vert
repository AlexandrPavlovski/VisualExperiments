#version 460 core

// zeros is replaced at runtime with an actual values
#define particlesCount 0
#define nBody 0


layout(std430) buffer;

layout(location = 0) uniform vec2 windowSize;
layout(location = 1) uniform float velocityDamping;
layout(location = 2) uniform float deltaTime;
layout(location = 3) uniform bool isPaused;
layout(location = 4) uniform float particleSize;
layout(location = 5) uniform uint cellSize;
layout(location = 6) uniform vec2 viewPos;
layout(location = 7) uniform float viewZoom;
layout(location = 8) uniform vec2 simulationArea;


layout(binding = 0) buffer SSBO
{
	vec2 particlesPos[];
};
layout(binding = 9) buffer tetete
{
	float test[];
};
layout(binding = 13) buffer SSBOPrevPos
{
	vec2 particlesPrevPos[];
};
//layout(binding = 14) buffer SSBOAcc
//{
//	vec2 particlesAcc[];
//};
layout(binding = 15) buffer SSBOPresure
{
	float particlesPresure[];
};
layout(binding = 16) buffer misc
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

void nBodyGravity(vec2 pos, inout vec2 pAcc)
{
	for (int i = 0; i < particlesCount; i += 4)
	{
		pAcc += getBodyToBodyAcceleration(pos, particlesPos[i]);
		pAcc += getBodyToBodyAcceleration(pos, particlesPos[i+1]);
		pAcc += getBodyToBodyAcceleration(pos, particlesPos[i+2]);
		pAcc += getBodyToBodyAcceleration(pos, particlesPos[i+3]);
	}
}

void updateParticle()
{
	vec2 pos = particlesPos[gl_VertexID];
	vec2 prevPos = particlesPrevPos[gl_VertexID];
	vec2 acc = vec2(0);

	if (!isPaused)
	{
		if (draggedParticleIndex - 1 != gl_VertexID)
		{
#if (nBody == 1)
			nBodyGravity(pos, acc);
#else
			const vec2 gravityAcc = vec2(0, 0.005);
			acc += gravityAcc;
#endif
			vec2 newPos = pos * 2 - prevPos + acc * deltaTime * deltaTime;
			vec2 vel = (newPos - pos) * velocityDamping;
			
//			float velLenghtSquared = dot(vel, vel);
//			if (velLenghtSquared > 15) vel *= 0.9;

			prevPos = newPos - vel;
			pos = newPos;
		}
	}
	
	particlesPos[gl_VertexID] = pos;
	particlesPrevPos[gl_VertexID] = prevPos;
	particlesPresure[gl_VertexID] = 0.0;
}

void setParticleColor()
{
	if (draggedParticleIndex - 1 == gl_VertexID)
	{
		particleColor = vec3(1.0, 0.0, 0.0);
		return;
	}

	float pressure = particlesPresure[gl_VertexID];

//pressure = float(gl_VertexID) / float(particlesCount);
	// gradient for pressure coloring
	vec3 firstColor =  vec3(0.0, 0.0, 1.0);
	vec3 middleColor = vec3(0.0, 1.0, 0.0);
	vec3 endColor =    vec3(1.0, 0.0, 0.0);

	float h = 0.5;
	vec3 col = mix(mix(firstColor, middleColor, pressure/h), mix(middleColor, endColor, (pressure - h)/(1.0 - h)), step(h, pressure));

	particleColor = vec3(col);
}

vec2 worldToScreen(vec2 particleWorldPos)
{
	vec2 windowCenter = windowSize / 2;
	vec2 particleNormalizedPos = (particleWorldPos + viewPos) / windowSize * viewZoom;
	vec2 particleScreenPos = particleNormalizedPos * 2 - 1;

	particleScreenPos.y = -particleScreenPos.y;

	return particleScreenPos;
}

void main()
{
	setParticleColor();
	updateParticle();
//test[gl_VertexID]=velLenghtSquared;
	vec2 screenPos = worldToScreen(particlesPos[gl_VertexID]);
	gl_Position = vec4(screenPos, 0.0, 1.0);
}
