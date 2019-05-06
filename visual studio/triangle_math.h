#pragma once
#include <OptiX_world.h>
#include <glm/glm.hpp>
#include "Vertex.h"
namespace triangle_math
{
	optix::float3 uv2xyz(int triangleId, optix::float2 &uv, std::vector<Vertex> &vertices);
	glm::vec3 calculateCentre(float triangleId, std::vector<Vertex> &vertices);
	glm::vec3 calculateCentre(std::vector<glm::vec3> triangle);
	float calculateSurface(std::vector<glm::vec3> triangle);
	float calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c);
	float calculateSurface(float triangleId, std::vector<Vertex> &vertices);
	bool isFacingBack(glm::vec3 origin, float destPatch, std::vector<Vertex> &vertices);
	glm::vec3 avgNormal(float triangleId, std::vector<Vertex> &vertices);
	float calcPointFormfactor(Vertex orig, Vertex dest, float surface);
	std::vector<std::vector<glm::vec3>> divideInFourTriangles(float triangleId, std::vector<Vertex> &vertices);
};

