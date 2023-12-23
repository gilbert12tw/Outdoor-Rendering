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

uniform vec4 viewPort;
uniform vec3 cameraPos;
uniform vec3 lookCenter;
uniform mat4 viewProjMat;

uniform sampler2D depth_map;

int type;
vec3 offset;
float radius;

// TODO: not working now
bool HizCulling(vec4 position){
    if (type >= 3) return true;
    vec4 pos = viewProjMat * vec4(position.xyz, 1.0);

    //pos.x += Frame.x / 2;
    //pos.y += Frame.y / 2;

    // viewport transform using viewPort
    pos.x = pos.x * viewPort.z + viewPort.x;
    pos.y = pos.y * viewPort.w + viewPort.y;


    /* divide by w to get normalized device coordinates */
    pos.xyz /= pos.w;

    float viewSizeX = 2.0 * radius * (viewPort.z / 2) / pos.w;
    float viewSizeY = 2.0 * radius * viewPort.w / pos.w;

    float depth = pos.z;
    depth = depth * 0.5 + 0.5;

    float LOD = ceil(log2(max(viewSizeX, viewSizeY) / 2.0));

    /* finally fetch the depth texture using explicit LOD lookups */
    vec4 Samples;
    Samples.x = textureLod( depth_map, vec2(pos.x - viewSizeX, pos.y - viewSizeY), LOD ).x;
    Samples.y = textureLod( depth_map, vec2(pos.x - viewSizeX, pos.y + viewSizeY), LOD ).x;
    Samples.z = textureLod( depth_map, vec2(pos.x + viewSizeX, pos.y - viewSizeY), LOD ).x;
    Samples.w = textureLod( depth_map, vec2(pos.x + viewSizeX, pos.y + viewSizeY), LOD ).x;
    float MaxDepth = max( max( Samples.x, Samples.y ), max( Samples.z, Samples.w ) );

    /* if the instance depth is bigger than the depth in the texture discard the instance */
    return ( depth > MaxDepth ) ? false : true;
}

bool furstrum_culling(vec4 position){
    if (distance(cameraPos, position.xyz) > 400.0) return false;


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

    if (idx < 567237) {
        type = 0;
        offset = vec3(0.0, 0.66, 0.0);
        radius = 1.4;
    } else if (idx < 567237 + 2797) {
        type = 1;
        offset = vec3(0.0, 2.55, 0.0);
        radius = 3.4;
    } else if (idx < 567237 + 2797 + 1010) {
        type = 2;
        offset = vec3(0.0, 1.76, 0.0);
        radius = 2.6;
    } else if (idx < 567237 + 2797 + 1010 + 298) {
        type = 3;
        offset = vec3(0.0, 4.57, 0.0);
        radius = 8.5;
    } else if (idx < 567237 + 2797 + 1010 + 298 + 299) {
        type = 4;
        offset = vec3(0.0, 4.57, 0.0);
        radius = 10.2;
    }

    if(furstrum_culling(instanceProps[idx].position) && HizCulling(instanceProps[idx].position)) {
        if (type == 0) {
            currValidInstanceIndex[atomicAdd(commands[0].instanceCount, 1)] = idx;
        } else if (type == 1) {
            currValidInstanceIndex[atomicAdd(commands[1].instanceCount, 1) + 567237] = idx;
        } else if (type == 2) {
            currValidInstanceIndex[atomicAdd(commands[2].instanceCount, 1) + 567237 + 2797] = idx;
        } else if (type == 3) {
            currValidInstanceIndex[atomicAdd(commands[3].instanceCount, 1) + 567237 + 2797 + 1010] = idx;
        } else if (type == 4) {
            currValidInstanceIndex[atomicAdd(commands[4].instanceCount, 1) + 567237 + 2797 + 1010 + 298] = idx;
        }
    }
}
