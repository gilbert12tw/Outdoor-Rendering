#version 450 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_uv_coord;
// layout (location = 3) in vec3 in_tangent;
out vec3 uv_coord;
out vec3 viewDirection;
out vec3 lightDirection;
out vec3 out_normal;

uniform vec3 offset;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform mat4 modelMat;

const vec3 lightDirection_world = vec3(0.4, 0.5, 0.8);

void main() {

	vec4 postion = viewMat * modelMat * vec4(vertex, 1.0);
	vec3 V = normalize(-postion.xyz);
	vec3 L = mat3(viewMat) * lightDirection_world - postion.xyz;

	uv_coord = in_uv_coord;
	viewDirection = V;
	lightDirection = L;
	out_normal = mat3(viewMat) * mat3(modelMat) * in_normal;

	gl_Position = projMat * postion;
}