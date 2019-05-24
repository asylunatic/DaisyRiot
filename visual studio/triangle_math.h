#pragma once
#include <OptiX_world.h>
#include <glm/glm.hpp>
#include "Vertex.h"
#include "MeshS.h"
namespace triangle_math
{
	optix::float3 uv2xyz(int triangleId, optix::float2 &uv, MeshS& mesh);
	glm::vec3 calculateCentre(float triangleId, MeshS& mesh);
	glm::vec3 calculateCentre(std::vector<glm::vec3> triangle);
	float calculateSurface(std::vector<glm::vec3> triangle);
	float calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c);
	float calculateSurface(float triangleId, MeshS& mesh);
	bool isFacingBack(glm::vec3 origin, float destPatch, MeshS& mesh);
	glm::vec3 avgNormal(float triangleId, MeshS& mesh);
	float calcPointFormfactor(vertex::Vertex orig, vertex::Vertex dest, float surface);
	std::vector<std::vector<glm::vec3>> divideInFourTriangles(float triangleId, MeshS& mesh);
};

