#version 450 core

// input from the rasterizer
in vec3 uv_coord;
in vec3 out_normal;
in vec3 world_pos;

out vec4 fragColor;
uniform sampler2D textureAirplane;
uniform sampler2D hiz_depth_map;

// output to the G-buffer
layout (location = 0) out vec4 color0; //Diffuse map
layout (location = 1) out vec4 color1; //Normal map
layout (location = 2) out vec4 color2; //Coordinate

void main() {
	vec4 texel = texture(textureAirplane, uv_coord.xy);

	color0 = texel;
	color1 = vec4(out_normal, 0.0);// w -> hi-z culling
	color2 = vec4(world_pos, 1.0); // w -> specular

	float depth = texture(hiz_depth_map, uv_coord.xy).x;

	//fragColor = vec4(pow(diffuse + specular + ambient, vec3(0.5)), 1.0);
	//fragColor = vec4(specular, 1.0);
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}