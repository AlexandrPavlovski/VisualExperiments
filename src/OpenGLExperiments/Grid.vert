#version 460 core


layout(location = 2) uniform vec2 simulationArea;
layout(location = 3) uniform vec2 viewPos;
layout(location = 4) uniform float viewZoom;
layout(location = 5) uniform vec2 windowSize;


void main() {
	vec2 trianglesData[4] = vec2[4]
	(
		vec2(0.0, 0.0),
		vec2(simulationArea.x, 0.0),
		vec2(0.0, simulationArea.y),
		vec2(simulationArea.x, simulationArea.y)
	);

	vec2 worldPos = trianglesData[gl_VertexID];
	vec2 worldNormPos = (worldPos + viewPos) / windowSize * viewZoom;
	vec2 screenPos = worldNormPos * 2 - 1;
	screenPos.y = -screenPos.y;

	gl_Position = vec4(screenPos, 0.0, 1.0);
}
