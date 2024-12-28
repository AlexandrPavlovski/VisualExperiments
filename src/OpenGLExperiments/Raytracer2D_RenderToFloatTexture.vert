#version 460 core

#define sqrt2 1.4142135623730950488

layout(location = 0) uniform ivec2 simulationArea;


layout(binding = 0) buffer SSBO
{
	vec2 linePoint[];
};


out float biasCorrection;
out vec4 pointColor;

void main() {
	int endindex = gl_VertexID % 2 == 0 ? gl_VertexID + 1 : gl_VertexID - 1;

	vec2 pStart = linePoint[gl_VertexID];
	vec2 pEnd = linePoint[endindex];

	vec2 dir = pEnd - pStart;
	float slope = min(abs(dir.x), abs(dir.y)) / max(abs(dir.x), abs(dir.y));
	biasCorrection = clamp(sqrt(slope * slope + 1), 1.0, sqrt2);
	
	gl_Position = vec4(pStart / (simulationArea * 0.5), 0.0, 1.0);
}
