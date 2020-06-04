#version 460 core

layout(location = 20) uniform vec4 uColor;

out vec4 color;

void main() {
	//color = vec4(0.196, 0.05, 0.941, 0.2);
	color = uColor;
}