#version 460 core

#define iterations 0
#define AA 0

layout(pixel_center_integer) in vec4 gl_FragCoord;


//layout(location = 0) uniform uint samplesWidth;
//layout(location = 1) uniform uint frameNumber;
//
//
//layout(binding = 0) buffer data
//{
//	float sampleIterations[];
//};


out vec4 color;

void main() {
	color = vec4(0.7, 0.3, 0.4, 1.0);
}
