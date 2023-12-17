#version 450 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_uv_coord;
layout (location = 3) in vec3 in_tangent;
out vec3 uv_coord;
out vec3 viewDirection;
out vec3 lightDirection;
out vec3 out_normal;

uniform mat4 viewMat;
uniform mat4 projMat;

const vec3 lightDirection_world = vec3(0.4, 0.5, 0.8);

void main() {

	vec3 offset = vec3(25.92, 18.27, 11.75);
	vec4 postion = viewMat * vec4(vertex + offset, 1.0);
	vec3 V = normalize(-postion.xyz);

	// TBN
	vec3 T = mat3(viewMat) * in_tangent;
	vec3 N = mat3(viewMat) * in_normal;
	vec3 B = mat3(viewMat) * cross(N, T);

	vec3 L = mat3(viewMat) * lightDirection_world - postion.xyz;

	uv_coord = in_uv_coord;
	viewDirection = vec3(dot(V, T), dot(V, B), dot(V, N));
	lightDirection = vec3(dot(L, T), dot(L, B), dot(L, N));

	gl_Position = projMat * postion;
}