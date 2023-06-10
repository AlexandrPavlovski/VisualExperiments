#version 460 core

// 0 is replaced at runtime with an actual value
#define particlesCount 0


layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform float cellSize;
layout(location = 1) uniform vec2 mousePos;
layout(location = 2) uniform int windowHeight;

struct collCell
{
	uint index;
	uint phantomAndHomeCellsCountPacked;
};

out vec4 color;

void main() {
	vec2 pixelGridPos = floor(gl_FragCoord.xy / cellSize);
	vec2 mouseGridPos = floor(mousePos / cellSize);

	color = pixelGridPos == mouseGridPos 
		? vec4(vec3(0.8), 1.0) // mouse highlight
		: vec4(mod(pixelGridPos.x + pixelGridPos.y, 2.0) * 0.5); // checker pattern
}
