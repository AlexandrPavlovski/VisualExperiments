#version 460 core

#define iterations 0
#define AA 0

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform uint samplesWidth;
layout(location = 1) uniform uint frameNumber;


layout(binding = 0) buffer data
{
	double sampleIterations[];
};


out vec4 color;

void main() {
	vec3 col = vec3(0.0);

	for (double m = 0; m < AA; m++)
	for (double n = 0; n < AA; n++)
	{
		dvec2 sampleCoord = gl_FragCoord.xy * AA + dvec2(n, m);
		int sampleIndex = int(sampleCoord.x + sampleCoord.y * samplesWidth) * 3;
		int zxIndex = sampleIndex + 1;
		int zyIndex = sampleIndex + 2;

		double i = sampleIterations[sampleIndex];
		if (i < frameNumber * iterations)
		{
			dvec2 z = dvec2(sampleIterations[zxIndex], sampleIterations[zyIndex]);
			float f = float(i) - log2(log2(float(dot(z,z)))) + 4.0;

			col += 0.5 + 0.5 * cos(3.0 + f * 0.15 + vec3(0.0, 0.6, 1.0));
		}
	}

	col /= float(AA * AA);
	color = vec4(col, 1.0);
}
