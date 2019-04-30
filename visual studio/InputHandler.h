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
	struct callback_context {
		callback_context(bool &left, bool &hitB, std::vector<Vertex> &debugline, int optixW, int optixH, optix::float3 &viewDirection, optix::float3 &eye, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
			std::vector<glm::vec3> &optixView, std::vector<optix_functionality::Hit> &patches, std::vector<Vertex> &vertices, std::vector<UV> &rands, OptixPrimeFunctionality& optixP, Eigen::VectorXf &lightningvalues, Eigen::SparseMatrix<float> &RadMat, Eigen::VectorXf &residualvector);
		bool &left;
		bool &hitB;
		std::vector<Vertex> &debugline;
		int optixW;
		int optixH;
		optix::float3 &viewDirection;
		optix::float3 &eye;
		std::vector<std::vector<MatrixIndex>> &trianglesonScreen;
		std::vector<glm::vec3> &optixView;
		std::vector<optix_functionality::Hit> &patches;
		std::vector<Vertex> &vertices;
		std::vector<UV> &rands;
		OptixPrimeFunctionality &optixP;
		Eigen::VectorXf &lightningvalues;
		Eigen::SparseMatrix<float> &RadMat;
		Eigen::VectorXf &residualvector;
	};
	
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
private:
	static callback_context* get_context(GLFWwindow* w);
	static void leftclick(GLFWwindow* window);
	static void move_left(GLFWwindow* window);
	static void move_right(GLFWwindow* window);
	static void move_up(GLFWwindow* window);
	static void move_down(GLFWwindow* window);
	static void calculate_form_vector(GLFWwindow* window);
	static void save_screenshot(GLFWwindow* window);
	static void find_triangle_by_id(GLFWwindow* window);
	static void set_radiosity_tex(GLFWwindow* window);
	static void increment_lightpasses(GLFWwindow* window);
};

