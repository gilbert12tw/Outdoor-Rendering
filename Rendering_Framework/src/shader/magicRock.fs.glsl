#version 450 core

// input from the rasterizer
in vec3 uv_coord;
in vec3 out_normal;
in vec3 world_pos;
in mat3 TBN;

out vec4 fragColor;

uniform sampler2D textureRock;
uniform sampler2D normalMap;

const vec3 specularLight = vec3(0.16, 0.16, 0.16);
const vec3 ambientLight = vec3(0.2, 0.2, 0.2);
const vec3 diffuseLight = vec3(0.64, 0.64, 0.64);

const float intensity = 1.0;
uniform int normalMaping;

// output to the G-buffer
layout (location = 0) out vec4 color0; //Diffuse map
layout (location = 1) out vec4 color1; //Normal map
layout (location = 2) out vec4 color2; //Coordinate

void main() {
	vec4 texel = texture(textureRock, uv_coord.xy);
	vec3 normal;

	if (normalMaping == 1) {
		vec3 normalMapTexel = texture(normalMap, uv_coord.xy).xyz;
		normal = normalMapTexel * 2.0 - 1.0;
		normal = (TBN * (normal));
	} else {
		normal = out_normal;
	}

	color0 = texel;
	color1 = vec4(normal, 1.0);    // w -> hi-z culling
	color2 = vec4(world_pos, 1.0); // w -> specular

	//fragColor = vec4(pow(diffuse + specular + ambient, vec3(0.5)), 1.0);
	//fragColor = vec4(N, 1.0);
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}