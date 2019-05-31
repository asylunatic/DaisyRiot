#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Vertex.h"
#include "Material.h"

class MeshS
{
public:
	//vertices: the unique vertices in the mesh
	//normals: the unique normals in the mesh
	//triangleIndices: triangles indexing to vertices and normals
	//trianglesPerVertices: an array, indexed per vertex like uniqvertices, pointing to triangles it is part of
	int numtriangles;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<vertex::TriangleIndex> triangleIndices;
	std::vector<std::vector<int>> trianglesPerVertex;
	std::vector<Material> materials;
	std::vector<int> materialIndexPerTriangle;

	void loadFromFile(char* filepath, char* mtlpath);
	MeshS(char* filepath, char* mtlpath);
	MeshS();
	~MeshS();
};

