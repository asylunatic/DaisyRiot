#ifndef VERTEX_H_
#define VERTEX_H_


// Library for vertex and matrix math
#include <glm/glm.hpp>

// Per-vertex data
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
};

#endif