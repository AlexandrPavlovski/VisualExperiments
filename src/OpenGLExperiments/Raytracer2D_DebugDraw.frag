#version 460 core

layout(location = 1) uniform vec4 c;


layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) out vec4 color;


void main() {
//	color = vec4(1.0);
	color = c;
}
