#pragma once;

#include "Vertex.h"
#include "Defines.h"
#include <chrono>

//#include "cuda_runtime.h"
//#include "device_launch_parameters.h"

#ifdef __CUDACC__
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#endif


#include <math_constants.h>
#include <stdio.h>
#include <iostream>

#define cudaCheckError() {                                          \
 cudaError_t e= cudaGetLastError();                                 \
 if(e!=cudaSuccess) {                                              \
   printf("Cuda failure %s:%d: '%s'\n",__FILE__,__LINE__,cudaGetErrorString(e));           \
  }                                                                 \
}

namespace parallellism
{
	struct Tripl {
		int m_row, m_col;
		double m_value;
	};

	std::vector<parallellism::Tripl>  runCalculateRadiosityMatrix(SimpleMesh& mesh);

	__global__
	void calculateRow(int row, Tripl* rowTripletList,
		glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices, int numtriangles);
	__device__
	float p2pFormfactor(int originPatch, int destPatch,
		glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices);
	__device__
	void divideInFourTriangles(glm::vec3 res[4][3], int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices);
	__device__
	glm::vec3 calculateCentre(int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices);
	__device__
	glm::vec3 calculateCentre(glm::vec3* triangle);
	__device__
	glm::vec3 avgNormal(int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices);
	__device__
	float calcPointFormfactor(vertex::Vertex orig, vertex::Vertex dest, float surface);
	__device__
	float calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c);
	__device__
	float calculateSurface(glm::vec3* triangle);
	__device__
	float calculateSurface(int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices);
}