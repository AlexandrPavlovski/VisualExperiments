#version 460 core

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(location = 0) uniform sampler2D floatTexture;
layout(location = 1) uniform uint frame;


in vec2 TexCoord;
out vec4 color;

void main() {
//	color = vec4(0.0, 0.0, 0.1, 1.0) + texture2D(floatTexture, gl_FragCoord.xy);
//	vec4 t = texture2D(floatTexture, gl_FragCoord.xy / 100.0);
	vec4 t = texture2D(floatTexture, TexCoord) / (10.0 * frame);
//	vec2 t = textureSize(floatTexture, 0) / 200.0;
	color = vec4(t.x,t.y,t.z, 1.0);
}
