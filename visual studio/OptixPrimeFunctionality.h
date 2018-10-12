#pragma once
#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include <ctime>
#include "Vertex.h"
#include "OptixFunctionality.h"
#include "Defines.h"
class OptixPrimeFunctionality
{
public:

	static void initOptixPrime(optix::prime::Context &contextP, optix::prime::Model &model, std::vector<Vertex> &vertices);
	static void doOptixPrime(int optixW, int optixH, optix::prime::Context &contextP, std::vector<glm::vec3> &optixView,
		optix::float3 &eye, optix::float3 &viewDirection, optix::prime::Model &model, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, std::vector<Vertex> &vertices);
	OptixPrimeFunctionality();
	~OptixPrimeFunctionality();
};

