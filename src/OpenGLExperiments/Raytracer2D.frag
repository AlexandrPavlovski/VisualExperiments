#version 460 core

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform sampler2D floatTexture;
layout(location = 1) uniform float exposure;
layout(location = 2) uniform ivec2 simulationArea;


in vec2 TexCoord;
out vec4 color;

void main() {
//	color = vec4(0.0, 0.0, 0.1, 1.0) + texture2D(floatTexture, gl_FragCoord.xy);
//	vec4 t = texture2D(floatTexture, gl_FragCoord.xy / 100.0);
	vec4 t = texture2D(floatTexture, TexCoord) * exposure;
//	vec2 t = textureSize(floatTexture, 0) / 200.0;
//	color = vec4(pow(t.xyz, vec3(1.0 / 2.2)), 1.0);
	color = vec4(t.xyz, 1.0);
}
