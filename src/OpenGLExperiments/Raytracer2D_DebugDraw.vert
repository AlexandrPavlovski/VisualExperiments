#version 460 core

layout(location = 0) uniform ivec2 simulationArea;


layout(binding = 1) buffer SSBOLines
{
	vec2 linePoints[];
};
vec2 trianglesCoords[4] = vec2[4]
(
	vec2(-10.0, 10.0),
	vec2(10.0, 10.0),
	vec2(-10.0, -10.0),
	vec2(10.0, -10.0)
);

void main() {
	gl_Position = vec4(linePoints[gl_VertexID] / (simulationArea * 0.5), 0.0, 1.0);
}
