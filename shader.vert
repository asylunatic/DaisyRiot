#version 430

// Model/view/projection matrix
layout(location = 0) uniform mat4 mvp;

// Per-vertex attributes
layout(location = 0) in vec3 pos; // World-space position
layout(location = 1) in vec3 normal; // World-space normal

// Data to pass to fragment shader
out vec3 fragPos;
out vec3 fragNormal;

void main() {
	// Transform 3D position into on-screen position
    gl_Position = mvp * vec4(pos, 1.0);

    // Pass position and normal through to fragment shader
    fragPos = pos;
    fragNormal = normal;
}