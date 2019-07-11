#include "MeshS.h"

MeshS::MeshS(){
	vertices = std::vector<glm::vec3>();
	normals = std::vector<glm::vec3>();
	triangleIndices = std::vector<vertex::TriangleIndex>();
	trianglesPerVertex = std::vector<std::vector<int>>();
	materials = std::vector<Material>();
	materialIndexPerTriangle = std::vector<int>();
}

MeshS::MeshS(char* filepath, char* mtlpath, std::vector<float> &wavelengths){
	vertices = std::vector<glm::vec3>();
	normals = std::vector<glm::vec3>();
	triangleIndices = std::vector<vertex::TriangleIndex>();
	trianglesPerVertex = std::vector<std::vector<int>>();
	materials = std::vector<Material>();
	materialIndexPerTriangle = std::vector<int>();
	loadFromFile(filepath, mtlpath, wavelengths);
}

void MeshS::loadFromFile(char * filepath, char * mtldirpath, std::vector<float> &wavelengths)
{
	// Load vertices of model
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> tinyobj_materials;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &tinyobj_materials, &err, filepath, mtldirpath, false)) {
		std::cerr << err << std::endl;
	}

	trianglesPerVertex.resize(attrib.vertices.size() / 3);

	// Get Materials
	for (int i = 0; i < tinyobj_materials.size(); i++){
		glm::vec3 diffuse(tinyobj_materials[i].diffuse[0], tinyobj_materials[i].diffuse[1], tinyobj_materials[i].diffuse[2]);
		glm::vec3 emission(tinyobj_materials[i].emission[0], tinyobj_materials[i].emission[1], tinyobj_materials[i].emission[2]);
		glm::vec3 blacklight(tinyobj_materials[i].specular[0], tinyobj_materials[i].specular[1], tinyobj_materials[i].specular[2]);
		
		bool blacklightsource = (emission[0] + emission[1] + emission[2] > 0 && diffuse[0] + diffuse[1] + diffuse[2] == 0.0);
		Material mattie = blacklightsource ? UVLightMaterial(diffuse, emission, blacklight, wavelengths) : Material(diffuse, emission, blacklight, wavelengths);

		materials.push_back(mattie);
	}

	// Get vertices
	for (int i = 0; i < attrib.vertices.size(); i += 3) {
		vertices.push_back({
			attrib.vertices[i],
			attrib.vertices[i + 1],
			attrib.vertices[i + 2]
		});
	}

	// Get normals
	for (int i = 0; i < attrib.normals.size(); i += 3) {
		normals.push_back({
			attrib.normals[i],
			attrib.normals[i + 1],
			attrib.normals[i + 2]
		});
	}

	// Read triangle vertices from OBJ file
	// Loop over each shape (consists of multiple triangles):
	int currTriangleIndex = 0;
	for (int i = 0; i < shapes.size(); i++) {
		tinyobj::shape_t &shape = shapes[i];

		// Loop over each triangle in shape:
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

			// uncomment for artistic rendering
			/*trianglesPerVertex[shape.mesh.indices[0].vertex_index].push_back(i);
			trianglesPerVertex[shape.mesh.indices[1].vertex_index].push_back(i);
			trianglesPerVertex[shape.mesh.indices[2].vertex_index].push_back(i);*/

			trianglesPerVertex[shape.mesh.indices[j + 0].vertex_index].push_back(currTriangleIndex);
			trianglesPerVertex[shape.mesh.indices[j + 1].vertex_index].push_back(currTriangleIndex);
			trianglesPerVertex[shape.mesh.indices[j + 2].vertex_index].push_back(currTriangleIndex);

			triangleIndices.push_back(triangle);

			// add material
			materialIndexPerTriangle.push_back(shape.mesh.material_ids[j/3]);
			
			currTriangleIndex += 1;
		}

	}
	numtriangles = triangleIndices.size();
	std::cout << "num of mat indices loaded: " << materialIndexPerTriangle.size() << std::endl;
}



MeshS::~MeshS()
{
}
