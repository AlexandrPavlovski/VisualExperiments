// most of the code is taken form here: https://www.shadertoy.com/view/4df3Rn

#version 460 core

// this values are replaced at runtime with an actual value
#define iterations 0
#define AA 0

#define iteration {                                \
    z = a3*cpow(z, 3.0) + a2*cmul(z, z) + a1*z + c;\
    if (dot(z, z) > (B * B)) break;                \
    l += 1.0;                                      \
}
// simple mandelbrot
//    z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;


layout(location = 0) uniform dvec2 uViewPos;
layout(location = 1) uniform double uViewZoom;
layout(location = 2) uniform float a1;
layout(location = 3) uniform float a2;
layout(location = 4) uniform float a3;

out vec4 color;

// complex numbers operations
vec2 cmul(vec2 a, vec2 b)  { return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x); }
vec2 cadd(vec2 a, float s) { return vec2(a.x+s, a.y); }
vec2 cpow(vec2 z, float n) { float r = length(z); float a = atan(z.y, z.x); return pow(r, n) * vec2(cos(a*n), sin(a*n)); }
// --------------------------

float fractal(in vec2 c)
{
    const float B = 256.0;
    float l = 0.0;
    vec2 z  = vec2(0.0);
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
    return l - log2(log2(dot(z,z))) + 4.0;
}

void main() {
    vec3 col = vec3(0.0);

    for (int m = 0; m < AA; m++)
    for (int n = 0; n < AA; n++)
    {
        vec2 point = gl_FragCoord.xy + vec2(float(m), float(n)) / float(AA);
	    vec2 c = vec2(point / float(uViewZoom)) + vec2(uViewPos);
        float l = fractal(c);
	    col += 0.5 + 0.5 * cos(3.0 + l * 0.15 + vec3(0.0, 0.6, 1.0));
	}
    col /= float(AA * AA);

    color = vec4(col, 1.0);
}
