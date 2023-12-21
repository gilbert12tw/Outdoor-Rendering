#version 450 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_uv_coord;
// layout (location = 3) in vec3 in_tangent;
out vec3 uv_coord;
out vec3 out_normal;
out vec3 world_pos;

uniform mat4 viewMat;
uniform mat4 projMat;
uniform mat4 modelMat;


void main() {
	vec4 postion = modelMat * vec4(vertex, 1.0);
	uv_coord = in_uv_coord;
	out_normal = mat3(modelMat) * in_normal;
	world_pos = postion.xyz;

	gl_Position = projMat * viewMat * postion;
}