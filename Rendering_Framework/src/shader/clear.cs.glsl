#version 450 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in ;
struct DrawCommand{
    uint count ;
    uint instanceCount ;
    uint firstIndex ;
    uint baseVertex ;
    uint baseInstance ;
};

layout (std430, binding=3) buffer DrawCommandsBlock{
    DrawCommand commands[] ;
};

void main() {
    const uint idx = gl_LocalInvocationID.x;
    commands[0].instanceCount = 0;
    commands[1].instanceCount = 0;
    commands[2].instanceCount = 0;
    commands[3].instanceCount = 0;
    commands[4].instanceCount = 0;
}
