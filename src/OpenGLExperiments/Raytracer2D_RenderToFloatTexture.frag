#version 460 core

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) out vec4 color;


in float biasCorrection;
in vec4 pointColor;

void main() {
	color = vec4(biasCorrection, biasCorrection, biasCorrection, 1.0);
//	color = vec4(1.0);
//	color = pointColor;
}