#pragma once
#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include <ctime>
#include <glm/glm.hpp>
#include <Eigen/Sparse>
#include <random>

#include "Vertex.h"
#include "optix_functionality.h"
#include "triangle_math.h"
#include "Defines.h"
#include "Drawer.h"

typedef Eigen::SparseMatrix<float> SpMat;
typedef Eigen::Triplet<double> Tripl;

class OptixPrimeFunctionality
{
public:
	optix::prime::Model model;
	optix::prime::Context contextP;
	bool intersectMouse(bool &left, double xpos, double ypos, int optixW, int optixH, optix::float3 &viewDirection, optix::float3 &eye, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
		std::vector<glm::vec3> &optixView, std::vector<optix_functionality::Hit> &patches, vertex::MeshS& mesh);
	bool shootPatchRay(std::vector<optix_functionality::Hit> &patches, vertex::MeshS& mesh);
	float p2pFormfactor(int originPatch, int destPatch, vertex::MeshS& mesh, std::vector<UV> &rands);
	float p2pFormfactorNusselt(int originPatch, int destPatch, vertex::MeshS& mesh, std::vector<UV> &rands);
	float calculatePointLightVisibility(optix::float3 &lightpos, int patchindex, vertex::MeshS& mesh, std::vector<UV> &rands);
	void calculateRadiosityMatrix(SpMat &RadMat, vertex::MeshS& mesh, std::vector<UV> &rands);
	void calculateRadiosityMatrixStochastic(SpMat &RadMat, vertex::MeshS& mesh, std::vector<UV> &rands);
	void initOptixPrime(vertex::MeshS& mesh);
	void doOptixPrime(int optixW, int optixH, std::vector<glm::vec3> &optixView, optix::float3 &eye, optix::float3 &viewDirection,  std::vector<std::vector<MatrixIndex>> &trianglesonScreen, 
		vertex::MeshS& mesh);
	static float calculateVisibility(int originPatch, int destPatch, vertex::MeshS& mesh, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands);
};

