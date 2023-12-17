#version 450 core

// input from the rasterizer
in vec3 uv_coord;
// in vec3 normal;
in vec3 viewDirection;
in vec3 lightDirection;
in vec3 out_normal;

out vec4 fragColor;
uniform sampler2D textureAirplane;


const vec3 specularLight = vec3(0.16, 0.16, 0.16);
const vec3 ambientLight = vec3(0.2, 0.2, 0.2);
const vec3 diffuseLight = vec3(0.64, 0.64, 0.64);
const float intensity = 1.0;
uniform mat4 viewMat;

void main() {
	vec4 texel = texture(textureAirplane, uv_coord.xy);

	vec3 normal = normalize(out_normal);
	
	vec3 V = normalize(viewDirection);
	vec3 L = normalize(lightDirection);
	vec3 N = normalize(normal);
	vec3 R = reflect(-L, N);

	vec3 diffuse = texel.xyz * max(dot(N, L), 0.0) * diffuseLight;
	vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * specularLight;
	vec3 ambient = texel.xyz * ambientLight;

	fragColor = vec4(pow(diffuse + specular + ambient, vec3(0.5)), 1.0);
	//fragColor = vec4(specular, 1.0);
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}