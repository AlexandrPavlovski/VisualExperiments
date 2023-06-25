#version 460 core

layout(std430) buffer;

layout(location = 0) uniform vec2 windowSize;
layout(location = 1) uniform vec2 viewPos;
layout(location = 2) uniform float viewZoom;


layout(binding = 0) buffer SSBO
{
	vec2 particlesPos[];
};
layout(binding = 9) buffer tetete
{
	float test[];
};
layout(binding = 15) buffer SSBOPressure
{
	float particlesPressure[];
};
layout(binding = 16) buffer misc
{
	uint draggedParticleIndex;
};


layout(location = 0) out vec3 particleColor;


void setParticleColor()
{
	if (draggedParticleIndex - 1 == gl_VertexID)
	{
		particleColor = vec3(1.0, 0.0, 0.0);
		return;
	}

	float pressure = particlesPressure[gl_VertexID];

	// gradient for pressure coloring
	vec3 startColor =  vec3(0.0, 0.0, 1.0);
	vec3 middleColor = vec3(0.0, 1.0, 0.0);
	vec3 endColor =    vec3(1.0, 0.0, 0.0);

	float h = 0.5;
	vec3 col = mix(mix(startColor, middleColor, pressure/h), mix(middleColor, endColor, (pressure - h)/(1.0 - h)), step(h, pressure));

	particleColor = vec3(col);
}

void main()
{
	setParticleColor();

	vec2 particleWorldPos = particlesPos[gl_VertexID];

	vec2 windowCenter = windowSize / 2;
	vec2 particleNormalizedPos = (particleWorldPos + viewPos) / windowSize * viewZoom;
	vec2 particleScreenPos = particleNormalizedPos * 2 - 1;

	particleScreenPos.y = -particleScreenPos.y;

	gl_Position = vec4(particleScreenPos, 0.0, 1.0);
}
