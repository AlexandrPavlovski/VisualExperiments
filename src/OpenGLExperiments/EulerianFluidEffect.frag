#version 460 core

#define U_fieldInputBunding 0
#define V_fieldInputBunding 1

layout(pixel_center_integer) in vec4 gl_FragCoord;


uniform sampler2D U_fieldSampler;
//layout(location = 1) uniform sampler2D V_fieldSampler;


//layout(binding = U_fieldInputBunding, r32f) uniform image2D U_field;
//layout(binding = V_fieldInputBunding, r32f) uniform image2D V_field;
layout(binding = 4) buffer cellTypeData
{
	uint cellType[];
};

out vec4 color;

void main() {
	float r = texture(U_fieldSampler, vec2(3.001, 2.001)).x;
//	float g = abs(imageLoad(U_field, ivec2(3, 2)).x);
//	float r = abs(imageLoad(U_field, ivec2(gl_FragCoord / 4)).x);
//	float g = abs(imageLoad(V_field, ivec2(gl_FragCoord / 4)).x);
//	float r = abs(1.0 - cellType[int(gl_FragCoord.y) * 1280 + int(gl_FragCoord.x)]);
	color = vec4(r, r, 0.0, 1.0);
}
