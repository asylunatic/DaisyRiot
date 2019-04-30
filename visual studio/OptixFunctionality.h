#pragma once

#include <OptiX_world.h>
#include <glm/glm.hpp>
#include "Vertex.h"

namespace OptixFunctionality
{
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

