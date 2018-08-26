#version 430

// Global variables for lighting calculations
uniform vec3 viewPos;
uniform sampler2D texToon;
uniform float time;
uniform vec3 lightPos;
uniform float zmin;
uniform float zmax;

// Output for on-screen color
layout(location = 0) out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal


void main() {
	vec3 lightDir = lightPos - fragPos;
	float lightLength = length(lightDir);
	lightDir = lightDir/lightLength;
	float normlength = length(fragNormal);
	vec3 norm = fragNormal/normlength;
	float u = max(0, dot(norm, lightDir));
	
	
	float z = distance(viewPos, fragPos);
	float s = z/zmax;
	
	
	// output
    outColor = texture(texToon, vec2(u, s));
}