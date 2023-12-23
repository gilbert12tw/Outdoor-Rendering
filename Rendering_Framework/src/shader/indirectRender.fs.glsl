#version 450 core

// input from the rasterizer
in vec3 uv_coord;
in vec3 normal;
in vec3 worldPosition;

out vec4 fragColor;
uniform sampler2DArray textureArray;

const vec3 ambientLight = vec3(0.2, 0.2, 0.2);
const vec3 diffuseLight = vec3(0.64, 0.64, 0.64);

// output to the G-buffer
layout (location = 0) out vec4 color0; //Diffuse map
layout (location = 1) out vec4 color1; //Normal map
layout (location = 2) out vec4 color2; //Coordinate

void main() {
    vec4 texel = texture(textureArray, uv_coord);
    if (texel.a < 0.5) {
        discard;
    }

    color0 = vec4(texel.xyz, 1.0);
    if (uv_coord.z >= 3.0) { // bulding
        color1 = vec4(normalize(normal), 1.0); // w -> hi-z culling
    } else {
        color1 = vec4(normalize(normal), 0.0);
    }
    color2 = vec4(worldPosition, 0.0); // w -> specular
}