#pragma once

#include <OptiX_world.h>
#include <glm/glm.hpp>
#include "Vertex.h"

namespace OptixFunctionality
{
	struct TriangleMath
	{
		static optix::float3 TriangleMath::uv2xyz(int triangleId, optix::float2 &uv, std::vector<Vertex> &vertices);
		static glm::vec3 TriangleMath::calculateCentre(float triangleId, std::vector<Vertex> &vertices);
		static glm::vec3 TriangleMath::calculateCentre(std::vector<glm::vec3> triangle);
		static float TriangleMath::calculateSurface(std::vector<glm::vec3> triangle);
		static float TriangleMath::calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c);
		static bool TriangleMath::isFacingBack(glm::vec3 origin, float destPatch, std::vector<Vertex> &vertices);
		static glm::vec3 TriangleMath::avgNormal(float triangleId, std::vector<Vertex> &vertices);
		static float TriangleMath::calcPointFormfactor(Vertex orig, Vertex dest, float surface);
		static std::vector<std::vector<glm::vec3>> TriangleMath::divideInFourTriangles(float triangleId, std::vector<Vertex> &vertices);
	};
	struct Hit {
		float t;
		int triangleId;
		optix::float2 uv;
	};
	optix::float3 glm2optixf3(glm::vec3 v);
	glm::vec3 optix2glmf3(optix::float3 v);
	void initOptix(optix::Context &context, std::vector<Vertex> &vertices);
	void doOptix(optix::Context &context, double &xpos, double &ypos);
};

