#version 460 core

#define U_fieldInputBinding 2
#define V_fieldInputBinding 3
#define smoke_fieldInputBinding 5

#define U_fieldBufferI UO
#define V_fieldBufferI VO
#define smoke_fieldBufferI SO

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform vec2 simulationArea;
layout(location = 1) uniform vec2 windowSize;

layout(binding = U_fieldInputBinding) buffer U_fieldBufferI { float U_field[]; };
layout(binding = V_fieldInputBinding) buffer V_fieldBufferI { float V_field[]; };
layout(binding = smoke_fieldInputBinding) buffer smoke_fieldBufferI { float smoke_fieldInput[]; };

layout(binding = 6) buffer cellTypeData
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

	vec2 fc = gl_FragCoord.xy;

	float sr = simulationArea.x / float(simulationArea.y); // simulation ratio
	float wr = windowSize.x / float(windowSize.y); // window ratio
	float rr = wr / sr; // ratio ratio
	if (sr < wr)
	{
		fc.x -= (windowSize.x - windowSize.x / rr) / 2;
		fc *= simulationArea / windowSize;
		fc.x *= rr;
	}
	else
	{
		fc.y -= (windowSize.y - windowSize.y * rr) / 2;
		fc *= simulationArea / windowSize;
		fc.y /= rr;
	}

	fc = floor(fc);

	int cellId = int(fc.y * simulationArea.x + fc.x);

//	float r = sampleField(true, gl_FragCoord.xy / 100);
//	float u = abs(imageLoad(U_field, ivec2(gl_FragCoord / 1)).x) / 1;
//	float v = abs(imageLoad(V_field, ivec2(gl_FragCoord / 1)).x) / 1;
	float b = abs(1.0 - cellType[cellId]);

	float u = abs(U_field[cellId]);
	float v = abs(V_field[cellId]);

	float s = smoke_fieldInput[cellId];

	color = vec4(s, s, s, 1.0);
//if (fc.y == 0) color = vec4(1);
}
