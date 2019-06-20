#pragma once

struct UV {
	float u;
	float v;
};

struct MatrixIndex {
	int col;
	int row;
	UV uv;
};

struct SimpleMesh {
	//vertices: the unique vertices in the mesh
	//normals: the unique normals in the mesh
	//triangleIndices: triangles indexing to vertices and normals
	//trianglesPerVertices: an array, indexed per vertex like uniqvertices, pointing to triangles it is part of
	int numtriangles;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<vertex::TriangleIndex> triangleIndices;
};

#define RAYS_PER_PATCH 100