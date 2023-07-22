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

layout(location = 0) uniform sampler2D texUnit;


layout(r32f, binding = 0) uniform image2D U_field;


out vec4 color;

void main() {
	float r = texture(texUnit, vec2(gl_FragCoord / 100)).x;
//	float r = imageLoad(U_field, ivec2(gl_FragCoord / 100)).x;
	color = vec4(r, r, r, 1.0);
}
