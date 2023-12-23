#version 450 core

layout (location = 0) in vec3 vertex;
out vec2 TexCoord;

void main(void) {
    gl_Position = vec4(vertex, 1.0);
    if (vertex.x == 1.0f && vertex.y == 1.0f) {
        TexCoord = vec2(1.0f, 1.0f);
    } else if (vertex.x == -1.0f && vertex.y == 1.0f) {
        TexCoord = vec2(0.0f, 1.0f);
    } else if (vertex.x == -1.0f && vertex.y == -1.0f) {
        TexCoord = vec2(0.0f, 0.0f);
    } else if (vertex.x == 1.0f && vertex.y == -1.0f) {
        TexCoord = vec2(1.0f, 0.0f);
    }
}
