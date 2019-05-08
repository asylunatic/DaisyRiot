#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <iostream>
// Library for loading .OBJ model
#include <tiny_obj_loader.h>

namespace vertex
{
	struct TriangleIndex{
		glm::ivec3 pos;
		glm::ivec3 normal;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
	};

	struct MeshS {
		//vertices: the unique vertices in the mesh
		//normals: the uniquenormals in the mesh
		//triangleIndices: triangles indexing to vertices and normals
		//trianglesPerVertices: an array, indexed per vertex like uniqvertices, pointing to triangles it is part of

		std::vector<glm::vec3>& vertices;
		std::vector<glm::vec3>& normals;
		std::vector<vertex::TriangleIndex>& triangleIndices;
		std::vector<std::vector<int>>& trianglesPerVertex;
	};

	void loadVertices(vertex::MeshS& mesh, char* filepath);
};

