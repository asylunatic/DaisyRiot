#version 430

// Per-vertex attributes
layout(location = 0) in vec3 pos; // World-space position

// Data to pass to fragment shader
out vec3 fragPos;
out vec3 fragNormal;

void main() {
	// Transform 3D position into on-screen position
    //gl_Position = mvp*(vec4(pos, 1.0) + vec4(0.0,-(cos(pos.x+5*time)*0.5 ),0.0,0.0)); 
	gl_Position = (vec4(pos, 1.0));
    // Pass position and normal through to fragment shader
    fragPos = pos;
    fragNormal = vec3(0,1,0);
}