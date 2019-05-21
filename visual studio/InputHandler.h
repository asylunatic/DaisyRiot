#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Drawer.h"
#include "ImageExporter.h"
#include "OptixPrimeFunctionality.h"
#include "Camera.h"
#include <chrono>

struct callback_context; 

class InputHandler
{
public:
	InputHandler();
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void cursor_pos_callback(GLFWwindow * window, double xpos, double ypos);

private:
	bool leftbuttonpressed;
	double old_x, old_y;
	callback_context* get_context(GLFWwindow* w);
	void leftclick(GLFWwindow* window);
	void leftrelease(GLFWwindow* window);
	void rightclick(GLFWwindow* window);
	void move_left(GLFWwindow* window);
	void move_right(GLFWwindow* window);
	void move_up(GLFWwindow* window);
	void move_down(GLFWwindow* window);
	void calculate_form_vector(GLFWwindow* window);
	void save_screenshot(GLFWwindow* window);
	void find_triangle_by_id(GLFWwindow* window);
	void toggle_view(GLFWwindow* window);
	void increment_lightpasses(GLFWwindow* window);
	void clear_light(GLFWwindow* window);
	void calc_full_lightning(GLFWwindow* window);
	void zoom_out(GLFWwindow* window);
	void zoom_in(GLFWwindow* window);
	void recalculate_screen(callback_context* cbc_ptr);
	void print_menu();
};

struct callback_context {
	callback_context(Drawer::DebugLine &debugline, Camera &camera, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, std::vector<glm::vec3> &optixView, std::vector<optix_functionality::Hit> &patches,
		vertex::MeshS& mesh, std::vector<UV> &rands, OptixPrimeFunctionality& optixP, Eigen::VectorXf &lightningvalues, Eigen::SparseMatrix<float> &RadMat, Eigen::VectorXf &emission,
		int &numpasses, Eigen::VectorXf &residualvector, bool &radiosityRendering, InputHandler &inputhandler);
	InputHandler inputhandler;
	Drawer::DebugLine &debugline;
	Camera &camera;
	std::vector<std::vector<MatrixIndex>> &trianglesonScreen;
	std::vector<glm::vec3> &optixView;
	std::vector<optix_functionality::Hit> &patches;
	vertex::MeshS& mesh;
	std::vector<UV> &rands;
	OptixPrimeFunctionality &optixP;
	Eigen::VectorXf &lightningvalues;
	Eigen::SparseMatrix<float> &RadMat;
	Eigen::VectorXf &emission;
	int numpasses;
	Eigen::VectorXf &residualvector;
	bool radiosityRendering;
};
