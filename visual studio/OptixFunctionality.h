#pragma once

#include <OptiX_world.h>
#include <glm/glm.hpp>
#include "Vertex.h"

class OptixFunctionality
{
public:
	struct TriangleMath
	{
		static optix::float3 TriangleMath::uv2xyz(int triangleId, optix::float2 &uv, std::vector<Vertex> &vertices);
		static glm::vec3 TriangleMath::calculateCentre(float triangleId, std::vector<Vertex> &vertices);
		static float TriangleMath::calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c);
		static glm::vec3 TriangleMath::avgNormal(float triangleId, std::vector<Vertex> &vertices);
	};
	struct Hit {
		float t;
		int triangleId;
		optix::float2 uv;
	};
	static optix::float3 glm2optixf3(glm::vec3 v);
	static glm::vec3 optix2glmf3(optix::float3 v);
	static void initOptix(optix::Context &context, std::vector<Vertex> &vertices);
	static void doOptix(optix::Context &context, double &xpos, double &ypos);
	OptixFunctionality();
	~OptixFunctionality();
};

