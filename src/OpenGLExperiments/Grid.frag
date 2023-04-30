#version 460 core

// 0 is replaced at runtime with an actual value
#define particlesCount 0

layout(location = 0) uniform float cellSize;
layout(location = 1) uniform vec2 mousePos;
layout(location = 2) uniform int windowHeight;

struct collCell
{
	uint index;
	uint phantomAndHomeCellsCountPacked;
};

layout(binding = 10) buffer collisionList
{
	collCell collisionCells[];
};

out vec4 color;

void main() {
	vec2 fragCoord = vec2(gl_FragCoord.x, windowHeight - gl_FragCoord.y); // moving origin to top left

	vec2 pixelGridPos = floor(fragCoord / cellSize);
	vec2 mouseGridPos = floor(mousePos / cellSize);

	color = pixelGridPos == mouseGridPos 
		? vec4(vec3(0.8), 1.0) // mouse highlight
		: vec4(mod(pixelGridPos.x + pixelGridPos.y, 2.0) * 0.5); // checker pattern
}
