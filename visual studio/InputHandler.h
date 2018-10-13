#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Drawer.h"
#include "ImageExporter.h"
#include "OptixPrimeFunctionality.h"

class InputHandler
{
public:
	static void leftclick(GLFWwindow* window, bool &left, bool &hitB, std::vector<Vertex> &debugline, int optixW, int optixH, optix::float3 &viewDirection, optix::float3 &eye, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
		optix::prime::Model &model, std::vector<glm::vec3> &optixView, std::vector<OptixFunctionality::Hit> &patches, std::vector<Vertex> &vertices);
};

