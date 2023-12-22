#version 430 core

in vec3 f_viewVertex ;
in vec3 f_uv ;
in vec3 f_worldVertex;
in vec3 f_worldNormal;

//layout (location = 0) out vec4 fragColor ;

layout(location = 2) uniform int pixelProcessId;
layout(location = 4) uniform sampler2D albedoTexture ;

// output to the G-buffer
layout (location = 0) out vec4 color0; //Diffuse map
layout (location = 1) out vec4 color1; //Normal map
layout (location = 2) out vec4 color2; //Coordinate

vec4 withFog(vec4 color){
	const vec4 FOG_COLOR = vec4(0.0, 0.0, 0.0, 1) ;
	const float MAX_DIST = 400.0 ;
	const float MIN_DIST = 350.0 ;
	
	float dis = length(f_viewVertex) ;
	float fogFactor = (MAX_DIST - dis) / (MAX_DIST - MIN_DIST) ;
	fogFactor = clamp(fogFactor, 0.0, 1.0) ;
	fogFactor = fogFactor * fogFactor ;
	
	vec4 colorWithFog = mix(FOG_COLOR, color, fogFactor) ;
	return colorWithFog ;
}


void terrainPass(){
	vec4 texel = texture(albedoTexture, f_uv.xy) ;
	//fragColor = withFog(texel);
	//fragColor.a = 1.0;
	color0 = vec4(withFog(texel).xyz, 1.0);
}

void pureColor(){
	//fragColor = withFog(vec4(1.0, 0.0, 0.0, 1.0)) ;
	color0 = withFog(vec4(1.0, 0.0, 0.0, 1.0)) ;
}

void main(){
	color1 = vec4(f_worldNormal, 1.0); // w -> hi-z culling
	color2 = vec4(f_worldVertex, 0.0); // w -> specular
	if(pixelProcessId == 5){
		pureColor() ;
	}
	else if(pixelProcessId == 7){
		terrainPass() ;
	}
	else{
		pureColor() ;
	}
}