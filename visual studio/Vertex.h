#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <iostream>
// Library for loading .OBJ model
#include <tiny_obj_loader.h>

namespace vertex
{
	struct TriangleIndex{
		glm::ivec3 vertex;
		glm::ivec3 normal;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
	};

};

