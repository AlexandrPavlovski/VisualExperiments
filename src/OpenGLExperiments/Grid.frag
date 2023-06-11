#version 460 core

// 0 is replaced at runtime with an actual value
#define particlesCount 0


layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform float cellSize;
layout(location = 1) uniform vec2 mousePos;
layout(location = 2) uniform vec2 simulationArea;
layout(location = 3) uniform vec2 viewPos;
layout(location = 4) uniform float viewZoom;

out vec4 color;

void main() {
	vec2 fragWorldCoord = (gl_FragCoord.xy - viewPos * viewZoom) / viewZoom;
	vec2 mouseWorldPos = (mousePos - viewPos * viewZoom) / viewZoom;

	vec2 pixelGridPos = floor(fragWorldCoord / cellSize);
	vec2 mouseGridPos = floor(mouseWorldPos / cellSize);

	color = pixelGridPos == mouseGridPos 
		? vec4(vec3(0.8), 1.0) // mouse highlight
		: vec4(mod(pixelGridPos.x + pixelGridPos.y, 2.0) * 0.5); // checker pattern
}
