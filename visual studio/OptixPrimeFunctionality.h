#pragma once
#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include <ctime>
#include <glm/glm.hpp>
#include <Eigen/Sparse>
#include "Vertex.h"
#include "OptixFunctionality.h"
#include "Defines.h"
#include "Drawer.h"
typedef Eigen::SparseMatrix<float> SpMat;

class OptixPrimeFunctionality : public OptixFunctionality
{
public:
	static bool intersectMouse(bool &left, double xpos, double ypos, int optixW, int optixH, optix::float3 &viewDirection, optix::float3 &eye, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
		optix::prime::Model &model, std::vector<glm::vec3> &optixView, std::vector<Hit> &patches, std::vector<Vertex> &vertices);
	static bool shootPatchRay(std::vector<Hit> &patches, std::vector<Vertex> &vertices, optix::prime::Model &model);
	static float p2pFormfactor(int originPatch, int destPatch, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<Vertex> &vertices, std::vector<UV> &rands);
	static float p2pFormfactor2(int originPatch, int destPatch, std::vector<Vertex> &vertices, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands);
	static float calculatePointLightVisibility(optix::float3 &lightpos, int patchindex, std::vector<Vertex> &vertices, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands);
	static void calculateRadiosityMatrix(SpMat &RadMat, std::vector<Vertex> &vertices, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands);
	static void initOptixPrime(optix::prime::Context &contextP, optix::prime::Model &model, std::vector<Vertex> &vertices);
	static void doOptixPrime(int optixW, int optixH, optix::prime::Context &contextP, std::vector<glm::vec3> &optixView,
		optix::float3 &eye, optix::float3 &viewDirection, optix::prime::Model &model, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, std::vector<Vertex> &vertices);

};

