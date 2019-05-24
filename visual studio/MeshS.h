#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Vertex.h"

class MeshS
{
public:
	//vertices: the unique vertices in the mesh
	//normals: the unique normals in the mesh
	//triangleIndices: triangles indexing to vertices and normals
	//trianglesPerVertices: an array, indexed per vertex like uniqvertices, pointing to triangles it is part of

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<vertex::TriangleIndex> triangleIndices;
	std::vector<std::vector<int>> trianglesPerVertex;

	void loadVertices(char* filepath, char* mtlpath);
	MeshS();
	~MeshS();
};

