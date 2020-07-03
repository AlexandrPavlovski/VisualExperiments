// most of the code is taken form here: https://www.shadertoy.com/view/4df3Rn

#version 460 core

// this values are replaced at runtime with an actual value
#define iterations 0
#define AA 0

#define iteration {                               \
    z = dvec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;\
    if (dot(z, z) > (B * B)) break;               \
    l += 1.0;                                     \
}

layout(location = 0) uniform dvec2 uViewPos;
layout(location = 1) uniform double uViewZoom;

out vec4 color;

float fractal(in dvec2 c)
{
    const double B = 256.0;
    float l = 0.0;
    dvec2 z  = dvec2(0.0);
    for (int i = 0; i < iterations / 10; i++)
    {
        // loop unroll optimization
        iteration
        iteration
        iteration
        iteration
        iteration
        iteration
        iteration
        iteration
        iteration
        iteration
    }

    if (l > iterations - 1) return 0.0;

    return l - log2(log2(float(dot(z,z)))) + 4.0;
}

void main() {
    vec3 col = vec3(0.0);

    for (int m = 0; m < AA; m++)
    for (int n = 0; n < AA; n++)
    {
        dvec2 point = gl_FragCoord.xy + dvec2(double(m), double(n)) / double(AA);
	    dvec2 c = dvec2(point / uViewZoom) + uViewPos;
        float l = fractal(c);
	    col += 0.5 + 0.5 * cos(3.0 + l * 0.15 + vec3(0.0, 0.6, 1.0));
	}
    col /= float(AA * AA);

    color = vec4(col, 1.0);
}
