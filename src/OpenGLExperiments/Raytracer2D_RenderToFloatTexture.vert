#version 460 core

layout(location = 0) uniform ivec2 simulationArea;


layout(binding = 0) buffer SSBO
{
	vec2 linePoint[];
};

vec2 trianglesCoords[4] = vec2[4]
(
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, -1.0)
);


void main() {
	gl_Position = vec4(linePoint[gl_VertexID] / simulationArea, 0.0, 1.0);
//	vec2 pos = trianglesCoords[gl_VertexID];
//	gl_Position = vec4(pos / 1.1, 0.0, 1.0);
}