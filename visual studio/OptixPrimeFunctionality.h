#pragma once
#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include <ctime>
#include <glm/glm.hpp>
#include "Vertex.h"
#include "OptixFunctionality.h"
#include "Defines.h"
#include "Drawer.h"

class OptixPrimeFunctionality : public OptixFunctionality
{
public:
	optix::prime::Model model;
	optix::prime::Context contextP;

	bool intersectMouse(bool &left, double xpos, double ypos, int optixW, int optixH, optix::float3 &viewDirection, optix::float3 &eye, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
		 std::vector<glm::vec3> &optixView, std::vector<Hit> &patches, std::vector<Vertex> &vertices);
	bool shootPatchRay(std::vector<Hit> &patches, std::vector<Vertex> &vertices);
	float p2pFormfactor(int originPatch, int destPatch, std::vector<Vertex> &vertices, std::vector<UV> &rands);
	float p2pFormfactor2(int originPatch, int destPatch, std::vector<Vertex> &vertices, std::vector<UV> &rands);
	void initOptixPrime(std::vector<Vertex> &vertices);
	void doOptixPrime(int optixW, int optixH, std::vector<glm::vec3> &optixView,
		optix::float3 &eye, optix::float3 &viewDirection, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, std::vector<Vertex> &vertices);

};

