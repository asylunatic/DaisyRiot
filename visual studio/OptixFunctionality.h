#pragma once

#include <OptiX_world.h>
#include <glm/glm.hpp>
#include "Vertex.h"

class OptixFunctionality
{
public:
	struct Hit {
		float t;
		int triangleId;
		optix::float2 uv;
	};
	static void initOptix(optix::Context context, std::vector<Vertex> vertices);
	static void doOptix(optix::Context context, double xpos, double ypos);
	OptixFunctionality();
	~OptixFunctionality();
};

