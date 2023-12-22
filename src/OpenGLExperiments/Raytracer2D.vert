#version 460 core

vec2 trianglesCoords[4] = vec2[4]
(
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, -1.0)
);
vec2 textureCoords[4] = vec2[4]
(
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 0.0)
);

out vec2 TexCoord;

void main() {
	vec2 pos = trianglesCoords[gl_VertexID];
	gl_Position = vec4(pos, 0.0, 1.0);
	TexCoord = textureCoords[gl_VertexID];
}
