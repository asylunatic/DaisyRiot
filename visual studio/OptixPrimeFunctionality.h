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
#include "Camera.h"
#include "MeshS.h"

typedef Eigen::SparseMatrix<float> SpMat;
typedef Eigen::Triplet<double> Tripl;

class OptixPrimeFunctionality
{
public:
	OptixPrimeFunctionality(MeshS& mesh);
	optix::prime::Model model;
	optix::prime::Context contextP;
	bool intersectMouse(Drawer::DebugLine &debugline, double xpos, double ypos, Camera &camera, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
		std::vector<glm::vec3> &optixView, std::vector<optix_functionality::Hit> &patches, MeshS& mesh);
	bool shootPatchRay(std::vector<optix_functionality::Hit> &patches, MeshS& mesh);
	float p2pFormfactor(int originPatch, int destPatch, MeshS& mesh);
	float p2pFormfactorNusselt(int originPatch, int destPatch, MeshS& mesh);
	float calculatePointLightVisibility(optix::float3 &lightpos, int patchindex, MeshS& mesh);
	void calculateRadiosityMatrix(SpMat &RadMat, MeshS& mesh);
	void calculateRadiosityMatrixStochastic(SpMat &RadMat, MeshS& mesh);
	void optixQuery(int number_of_rays, std::vector<optix::float3> &rays, std::vector<optix_functionality::Hit> &hits);
	void traceScreen(Drawer::RenderContext rendercontext);
	float calculateVisibility(int originPatch, int destPatch, MeshS& mesh, optix::prime::Context &contextP, optix::prime::Model &model);
private:
	std::vector<UV> rands;
};

