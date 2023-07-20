#version 460 core

layout(pixel_center_integer) in vec4 gl_FragCoord;


out vec4 color;

void main() {
	color = vec4(0.7, 0.3, 0.4, 1.0);
}
