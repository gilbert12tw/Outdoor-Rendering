#version 450 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_uv_coord;
out vec3 uv_coord;
out vec3 normal;
out vec3 viewDirection;
out vec3 lightDirection;

uniform mat4 projMat;
uniform mat4 viewMat;

const vec3 lightDirection_world = vec3(0.4, 0.5, 0.8);

struct DrawCommand{
    uint count ;
    uint instanceCount ;
    uint firstIndex ;
    uint baseVertex ;
    uint baseInstance ;
};

struct InstanceProperties {
    vec4 position;
    vec4 modelMat[4];
};

layout (std430, binding=1) buffer InstanceData{
    InstanceProperties instance[] ;
};

layout (std430, binding=2) buffer CurrValidInstanceIndex{
    uint currValidInstanceIndex[] ;
};

layout (std430, binding=3) buffer DrawCommandsBlock{
    DrawCommand commands[] ;
};

/*
num sample : 567237
num sample : 2797
num sample : 1010
num sample : 298
num sample : 299
*/

void main() {
    uv_coord = in_uv_coord;

    uint instanceID;
    if (uv_coord.z <= 0.1) {
        instanceID = currValidInstanceIndex[gl_InstanceID];
    } else if (uv_coord.z <= 1.1) {
        instanceID = currValidInstanceIndex[gl_InstanceID + 567237];
    } else if (uv_coord.z <= 2.1) {
        instanceID = currValidInstanceIndex[gl_InstanceID + 567237 + 2797];
    } else if (uv_coord.z <= 3.1) {
        instanceID = currValidInstanceIndex[gl_InstanceID + 567237 + 2797 + 1010];
    } else if (uv_coord.z <= 4.1) {
        instanceID = currValidInstanceIndex[gl_InstanceID + 567237 + 2797 + 1010 + 298];
    }


    //mat4 modelMat = mat4(instance[instanceID].modelMat[0], instance[instanceID].modelMat[1], instance[instanceID].modelMat[2], instance[instanceID].modelMat[3]);
    mat4 modelMat = mat4(1.0);
    vec4 postion = viewMat * vec4(mat3(modelMat) * vertex + instance[instanceID].position.xyz, 1.0);

    normal = mat3(viewMat) * in_normal;
    vec3 V = (-postion.xyz);
    vec3 L = mat3(viewMat) * lightDirection_world - postion.xyz;

    viewDirection = V;
    lightDirection = L;

    gl_Position = projMat * postion;
}