#version 430

// Global variables for lighting calculations
uniform sampler2D texToon;


// Output for on-screen color
layout(location = 0) out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal


void main() {
	vec2 uv = vec2(fragPos.x, fragPos.y);
	uv = (uv + vec2(1, 1))/2;
	
	// output
    outColor = texture(texToon, uv);
}