#version 460 core

// this value is replaced at runtime with an actual value
#define iterations 0

layout(location = 0) uniform vec2 uViewPos;
layout(location = 1) uniform float uViewZoom;

out vec4 color;

void main() {
	int n = 0;
	vec2 z = vec2(0.0);
	vec2 c = vec2(gl_FragCoord.xy / uViewZoom) + uViewPos;

	for	(int i = 0; i < iterations; i++)
	{
		if ((z.x * z.x + z.y * z.y) < 4.0)
		{
			float re = z.x * z.x - z.y * z.y + c.x;
			float im = z.x * z.y * 2.0 + c.y;
			z.x = re;
			z.y = im;
			n++;
		}
	}

	if (n == iterations)
	{
		n = 0;
	}

	vec3 intensity = vec3(sqrt(float(n) / iterations));
	color = vec4(intensity, 1.0);
}