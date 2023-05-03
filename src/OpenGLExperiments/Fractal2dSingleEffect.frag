// most of the code is taken form here: https://www.shadertoy.com/view/4df3Rn

#version 460 core

// this values are replaced at runtime with an actual value
#define iterations 0
#define AA 0

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform dvec2 uViewPos;
layout(location = 1) uniform double uViewZoom;
layout(location = 2) uniform float a1;
layout(location = 3) uniform float a2;
layout(location = 4) uniform float a3;
layout(location = 5) uniform uint samplesWidth;
layout(location = 6) uniform int windowHeight;
layout(location = 7) uniform uint frameNumber;


layout(binding = 0) buffer data
{
	float sampleIterations[];
};


out vec4 color;

vec3 hsv2rgb(vec3 c){
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	return c.z * mix(K.xxx, clamp(abs(fract(c.x + K.xyz) * 6.0 - K.w) - K.x, 0.0, 1.0), c.y);
}

vec3 hue2rgb(float hue){
	hue=fract(hue);
	return vec3(
		abs(hue*6.-3.)-1.,
		2.-abs(hue*6.-2.),
		2.-abs(hue*6.-4.)
	);
}

void main() {
//	vec3 col = vec3(0.0);

//    for (int m = 0; m < AA; m++)
//    for (int n = 0; n < AA; n++)
//    {
//        vec2 point = gl_FragCoord.xy + vec2(float(m), float(n)) / float(AA);
//	    vec2 c = vec2(point / float(uViewZoom)) + vec2(uViewPos);
//        float l = fractal(c);
//	    col += 0.5 + 0.5 * cos(3.0 + l * 0.15 + vec3(0.0, 0.6, 1.0));
//	}
//    col /= float(AA * AA);
//
//    color = vec4(col, 1.0);

	vec3 col = vec3(0.0);

	for (float m = 0; m < AA; m++)
	for (float n = 0; n < AA; n++)
	{
		vec2 sampleCoord = gl_FragCoord.xy * AA + vec2(n, m);
		int sampleIndex = int(sampleCoord.x + sampleCoord.y * samplesWidth) * 3;
		int zxIndex = sampleIndex + 1;
		int zyIndex = sampleIndex + 2;

		float i = sampleIterations[sampleIndex];
		if (i < frameNumber * iterations)
		{
			vec2 z = vec2(sampleIterations[zxIndex], sampleIterations[zyIndex]);
			float f = i - log2(log2(dot(z,z))) + 4.0;

			col += 0.5 + 0.5 * cos(3.0 + f * 0.15 + vec3(0.0, 0.6, 1.0));
//			col += hue2rgb(0.1 + 0.5 * cos(3.0 + f * 0.15)) * 0.3;
		}
	}

	col /= float(AA * AA);
	color = vec4(col, 1.0);
}
