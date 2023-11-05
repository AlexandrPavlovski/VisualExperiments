#version 460 core

#define U_fieldInputBunding 2
#define V_fieldInputBunding 3

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
//	ivec2 cellCoord = ivec2(gl_FragCoord);
//	ivec2 rightCellCoord = ivec2(cellCoord.x + 1, cellCoord.y);
//	ivec2 upCellCoord = ivec2(cellCoord.x, cellCoord.y + 1);
//	float u0 = imageLoad(U_field, cellCoord).x;
//	float u1 = imageLoad(U_field, rightCellCoord).x;
//	float v0 = imageLoad(V_field, cellCoord).x;
//	float v1 = imageLoad(V_field, upCellCoord).x;
//	float divergence = u1 - u0 + v1 - v0;

//	float r = sampleField(true, gl_FragCoord.xy / 100);
	float u = abs(imageLoad(U_field, ivec2(gl_FragCoord / 1)).x) / 1;
	float v = abs(imageLoad(V_field, ivec2(gl_FragCoord / 1)).x) / 1;
	float b = abs(1.0 - cellType[int(gl_FragCoord.y) * simulationArea.x + int(gl_FragCoord.x)]);

	color = vec4(v, u, 0, 1.0);
}
