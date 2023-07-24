#version 460 core

#define U_fieldInputBunding 0
#define V_fieldInputBunding 1

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform ivec2 simulationArea;

layout(binding = U_fieldInputBunding, r32f) uniform image2D U_field;
layout(binding = V_fieldInputBunding, r32f) uniform image2D V_field;
layout(binding = 4) buffer cellTypeData
{
	uint cellType[];
};

out vec4 color;

void main() {
//	float r = sampleField(true, gl_FragCoord.xy / 100);
	float r = abs(imageLoad(U_field, ivec2(gl_FragCoord)).x) / 10;
	float g = abs(imageLoad(V_field, ivec2(gl_FragCoord)).x) / 10;
	float b = abs(1.0 - cellType[int(gl_FragCoord.y) * simulationArea.x + int(gl_FragCoord.x)]);
	color = vec4(r + g, g + r, b, 1.0);
}
