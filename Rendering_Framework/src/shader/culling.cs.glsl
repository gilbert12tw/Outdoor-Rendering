#version 450 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in ;
struct DrawCommand{
    uint count ;
    uint instanceCount ;
    uint firstIndex ;
    uint baseVertex ;
    uint baseInstance ;
};

struct RawInstanceProperties{
    vec4 position ;
    ivec4 indices ;
};

struct InstanceProperties {
    vec4 position;
    mat4 modelMat;
};

layout (std430, binding=1) buffer InstanceData{
    InstanceProperties instanceProps[] ;
};

layout (std430, binding=2) buffer CurrValidInstanceIndex{
    uint currValidInstanceIndex[] ;
};

layout (std430, binding=3) buffer DrawCommandsBlock{
    DrawCommand commands[] ;
};

uniform vec3 cameraPos;
uniform vec3 lookCenter;
uniform mat4 viewProjMat;

bool furstrum_culling(vec4 position){
    if (distance(cameraPos, position.xyz) > 400.0) return false;

    vec3 offset = vec3(0.0, 0.66, 0.0);
    float radius = 1.4;

    if (position.w <= 0.1) {

    } else if (position.w <= 1.1) {
        offset = vec3(0.0, 2.55, 0.0);
        radius = 3.4;
    } else if (position.w <= 2.1) {
        offset = vec3(0.0, 1.76, 0.0);
        radius = 2.6;
    } else if (position.w <= 3.1) {
        offset = vec3(0.0, 4.57, 0.0);
        radius = 8.5;
    } else if (position.w <= 4.1) {
        offset = vec3(0.0, 4.57, 0.0);
        radius = 10.2;
    }

    position.xyz += offset;
    vec4 pos = viewProjMat * vec4(position.xyz, 1.0);


    // frustum culling
    if (pos.x > pos.w + radius || pos.x < -pos.w - radius) return false;
    if (pos.y > pos.w + radius || pos.y < -pos.w - radius) return false;
    if (pos.z > pos.w + radius || pos.z < -pos.w - radius) return false;

    return true;
}

/*
suample 0 : 567237
suample 1 : 2797
suample 2 : 1010
suample 3 : 298
suample 4 : 299
*/

void main() {
    const uint idx = gl_GlobalInvocationID.x;
    if(furstrum_culling(instanceProps[idx].position)) {
        if (idx < 567237) {
            currValidInstanceIndex[atomicAdd(commands[0].instanceCount, 1)] = idx;
        } else if (idx < 567237 + 2797) {
            currValidInstanceIndex[atomicAdd(commands[1].instanceCount, 1) + 567237] = idx;
        } else if (idx < 567237 + 2797 + 1010) {
            currValidInstanceIndex[atomicAdd(commands[2].instanceCount, 1) + 567237 + 2797] = idx;
        } else if (idx < 567237 + 2797 + 1010 + 298) {
            currValidInstanceIndex[atomicAdd(commands[3].instanceCount, 1) + 567237 + 2797 + 1010] = idx;
        } else if (idx < 567237 + 2797 + 1010 + 298 + 299) {
            currValidInstanceIndex[atomicAdd(commands[4].instanceCount, 1) + 567237 + 2797 + 1010 + 298] = idx;
        }
    }
}
