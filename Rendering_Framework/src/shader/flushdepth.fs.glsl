#version 450 core

uniform sampler2D depth_map;

void main(void) {
    float depth = texelFetch(depth_map, ivec2(gl_FragCoord.xy), 0).r;
    gl_FragDepth = depth;
}
