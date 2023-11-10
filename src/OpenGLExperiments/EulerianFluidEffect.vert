#version 460 core

layout(location = 0) uniform vec2 simulationArea;
layout(location = 1) uniform vec2 windowSize;


vec2 trianglesData[4] = vec2[4]
(
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, -1.0)
);

void main() {
	vec2 pos = trianglesData[gl_VertexID];

	// adjusting draw plane position in window to keep simulation area aspect ratio the same regerdless of window aspect ratio
	float sr = simulationArea.x / float(simulationArea.y); // simulation ratio
	float wr = windowSize.x / float(windowSize.y); // window ratio
	float rr = wr / sr; // ratio ratio
	if (sr < wr)
	{
		pos.x /= rr;
	}
	else
	{
		pos.y *= rr;
	}

	gl_Position = vec4(pos, 0.0, 1.0);
}
