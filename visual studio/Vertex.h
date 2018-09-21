#ifndef VERTEX_H_
#define VERTEX_H_
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
// Library for loading .OBJ model
#include <tiny_obj_loader.h>

class Vertex
{
public:
	glm::vec3 pos;
	glm::vec3 normal;
	Vertex();
	Vertex(glm::vec3 p, glm::vec3 n);
	~Vertex();
	static void loadVertices(std::vector<Vertex> &vertices, const char * filepath);
};
#endif

