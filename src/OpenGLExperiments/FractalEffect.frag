#version 460 core

// zero is replaced at runtime with an actual value
#define iterations 512

out vec4 color;

void main() {
	int n = 0;
	vec2 z = vec2(0.0);
	vec2 c = vec2(gl_FragCoord.x / 1280.0, gl_FragCoord.y / 800.0);

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

	float col = sqrt(float(n) / iterations);
	color = vec4(col, col, col, 1.0);
}