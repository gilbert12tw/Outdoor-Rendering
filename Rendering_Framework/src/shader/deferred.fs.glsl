#version 450 core

layout (location = 0) out vec4 fragColor;
layout (location = 0) uniform sampler2D diffuse_map; //Diffuse map
layout (location = 1) uniform sampler2D normal_map; //Normal map
layout (location = 2) uniform sampler2D position_map; //Coordinat
layout (location = 3) uniform sampler2D depth_map; // Depth

uniform vec3 eye_position;
uniform int gbuffer_mode;

const vec3 lightDirection_world = vec3(0.4, 0.5, 0.8);

const vec3 ambientLight = vec3(0.2, 0.2, 0.2);
const vec3 diffuseLight = vec3(0.64, 0.64, 0.64);
const vec3 specularLight = vec3(0.16, 0.16, 0.16);

void main(void)
{
    //fragColor = vec4(1.0, 0, 0, 1);
    //fragColor = texelFetch(ivec2(gl_FragCoord.xy), diffuse_map);
    vec3 diffuse = texelFetch(diffuse_map, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 normal = texelFetch(normal_map, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 position = texelFetch(position_map, ivec2(gl_FragCoord.xy), 0).rgb;
    float specular = texelFetch(position_map, ivec2(gl_FragCoord.xy), 0).w;

    if (gbuffer_mode == 1) {
        fragColor = vec4(normalize(position) * 0.5 + 0.5, 1.0);
        return;
    }
    else if (gbuffer_mode == 2) {
        fragColor = vec4(normalize(normal) * 0.5 + 0.5, 1.0);
        return;
    }
    else if (gbuffer_mode == 3) {
        fragColor = vec4(diffuse, 1.0);
        return;
    }
    else if (gbuffer_mode == 6) {
        float depth = texelFetch(depth_map, ivec2(gl_FragCoord.xy), 0).x;
        float near = 0.1;
        float far = 500.0;
        depth = (2.0 * near) / (far + near - depth * (far - near));
        fragColor = vec4(depth, depth, depth, 1.0);
        return;
    }

    // blinn-phong
    vec3 L = normalize(lightDirection_world);
    vec3 N = normalize(normal);
    vec3 V = normalize(eye_position - position);
    vec3 R = reflect(-L, N);

    vec3 diffuse_color = diffuse * diffuseLight * max(dot(N, L), 0.0);
    vec3 specular_color = specular * specularLight * pow(max(dot(N, R), 0.0), 32.0);
    vec3 ambient_color = diffuse * ambientLight;

    if (gbuffer_mode == 0) {
        fragColor = vec4(diffuse_color + specular_color + ambient_color, 1.0);
    } else if (gbuffer_mode == 4) {
        if (specular > 0.0)
            fragColor = vec4(1.0, 1.0, 1.0, 1.0);
        else
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else if (gbuffer_mode == 5) {
        fragColor = vec4(pow(diffuse_color + specular_color + ambient_color, vec3(0.5)), 1.0);
    }
}