#pragma once
#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include <ctime>
#include <glm/glm.hpp>
#include <Eigen/Sparse>
#include <random>
#include <chrono>

#include "Vertex.h"
#include "optix_functionality.h"
#include "triangle_math.h"
#include "Defines.h"
#include "Drawer.h"
#include "Camera.h"
#include "parallellism.cuh"

typedef Eigen::SparseMatrix<float> SpMat;
typedef Eigen::Triplet<double> Tripl;

class OptixPrimeFunctionality
{
public:

	OptixPrimeFunctionality(vertex::MeshS& mesh);
	optix::prime::Model model;
	optix::prime::Context contextP;

	void cudaCalculateRadiosityMatrix(SpMat &RadMat, vertex::MeshS& mesh, std::vector<UV> &rands);

	bool intersectMouse(Drawer::DebugLine &debugline, double xpos, double ypos, Camera &camera, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
		std::vector<glm::vec3> &optixView, std::vector<optix_functionality::Hit> &patches, vertex::MeshS& mesh);
	bool shootPatchRay(std::vector<optix_functionality::Hit> &patches, vertex::MeshS& mesh);
	float p2pFormfactor(int originPatch, int destPatch, vertex::MeshS& mesh, std::vector<UV> &rands);
	float p2pFormfactorNusselt(int originPatch, int destPatch, vertex::MeshS& mesh, std::vector<UV> &rands);
	float calculatePointLightVisibility(optix::float3 &lightpos, int patchindex, vertex::MeshS& mesh, std::vector<UV> &rands);
	void calculateRadiosityMatrix(SpMat &RadMat, vertex::MeshS& mesh, std::vector<UV> &rands);
	void calculateRadiosityMatrixStochastic(SpMat &RadMat, vertex::MeshS& mesh, std::vector<UV> &rands);
	void optixQuery(int number_of_rays, std::vector<optix::float3> &rays, std::vector<optix_functionality::Hit> &hits);
	void traceScreen(std::vector<glm::vec3> &optixView, Camera &camera, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
		vertex::MeshS& mesh);
	std::vector<Tripl> calculateAllVisibility(std::vector<parallellism::Tripl> &tripletlist, vertex::MeshS& mesh, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands);
	float calculateVisibility(int originPatch, int destPatch, vertex::MeshS& mesh, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands);
};

