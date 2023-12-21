#version 450 core

// input from the rasterizer
in vec3 uv_coord;
in vec3 normal;
in vec3 viewDirection;
in vec3 lightDirection;

out vec4 fragColor;
uniform sampler2DArray textureArray;

const vec3 ambientLight = vec3(0.2, 0.2, 0.2);
const vec3 diffuseLight = vec3(0.64, 0.64, 0.64);

void main() {
    vec4 texel = texture(textureArray, uv_coord);
    if (texel.a < 0.5) {
        discard;
    }

    vec3 V = normalize(viewDirection);
    vec3 L = normalize(lightDirection);
    vec3 N = normalize(normal);
    vec3 R = reflect(-L, N);

    vec3 diffuse = texel.xyz * max(dot(N, L), 0.0) * diffuseLight;
    vec3 ambient = texel.xyz * ambientLight;

    fragColor = vec4(ambient + diffuse, 1.0);
    // gamma correction
    fragColor = pow(fragColor, vec4(0.5));
}