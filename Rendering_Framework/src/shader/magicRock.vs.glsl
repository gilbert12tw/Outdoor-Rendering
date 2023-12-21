#version 450 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_uv_coord;
layout (location = 3) in vec3 in_tangent;

out vec3 uv_coord;
out vec3 out_normal;
out vec3 world_pos;
out mat3 TBN;

uniform vec3 cameraPos;
uniform mat4 viewMat;
uniform mat4 projMat;


void main() {
	vec3 offset = vec3(25.92, 18.27, 11.75);
	vec4 postion = vec4(vertex + offset, 1.0);
	world_pos = postion.xyz;
	out_normal = normalize(in_normal);
	uv_coord = in_uv_coord;

	// TBN
	vec3 T = normalize(in_tangent);
	vec3 N = normalize(in_normal);
	vec3 B = cross(N, T);
	TBN = mat3(T, B, N);

	gl_Position = projMat * viewMat * postion;
}