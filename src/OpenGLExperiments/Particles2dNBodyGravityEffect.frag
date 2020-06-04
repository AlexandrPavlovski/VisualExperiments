#version 460 core

layout(location = 20) uniform vec4 uColor;

out vec4 color;

void main() {
	color = uColor;
}