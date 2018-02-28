#version 430

uniform vec3 viewPos;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 fragPos;
out vec3 fragNormal;

layout(location = 0) uniform mat4 mvp;


void main() {	
vec4 mvppos = mvp*vec4(position, 1.0); 
gl_Position = mvppos;
fragPos = position;
fragNormal = normal;
}