#include "Vertex.h"



void vertex::loadVertices(vertex::MeshS& mesh, char * filepath)
{
	// Load vertices of model
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath)) {
		std::cerr << err << std::endl;
		//return EXIT_FAILURE;
	}

	mesh.trianglesPerVertex.resize(attrib.vertices.size()/3);

	for (int i = 0; i < attrib.vertices.size(); i += 3) {
		mesh.vertices.push_back({
			attrib.vertices[i],
			attrib.vertices[i + 1],
			attrib.vertices[i + 2]
		});
	}

	for (int i = 0; i < attrib.normals.size(); i += 3) {
		mesh.normals.push_back({
			attrib.normals[i],
			attrib.normals[i + 1],
			attrib.normals[i + 2]
		});
	}

	// Read triangle vertices from OBJ file
	//loop over each shape (consists of multiple triangles)
	int currTriangleIndex = 0;
	for (int i = 0; i < shapes.size(); i++) {
		tinyobj::shape_t shape = shapes[i];

		//loop over each triangle in shape
		for (int j = 0; j < shape.mesh.indices.size(); j += 3) {

			vertex::TriangleIndex triangle = {};
			triangle.vertex = {
				shape.mesh.indices[j + 0].vertex_index,
				shape.mesh.indices[j + 1].vertex_index,
				shape.mesh.indices[j + 2].vertex_index
			};

			triangle.normal = {
				shape.mesh.indices[j + 0].normal_index,
				shape.mesh.indices[j + 1].normal_index,
				shape.mesh.indices[j + 2].normal_index
			};

			mesh.trianglesPerVertex[shape.mesh.indices[j + 0].vertex_index].push_back(currTriangleIndex);
			mesh.trianglesPerVertex[shape.mesh.indices[j + 1].vertex_index].push_back(currTriangleIndex);
			mesh.trianglesPerVertex[shape.mesh.indices[j + 2].vertex_index].push_back(currTriangleIndex);

			currTriangleIndex += 1;
			mesh.triangleIndices.push_back(triangle);
		}
	}
}
