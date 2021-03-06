﻿// Library for OpenGL function loading
// Must be included before GLFW
#define GLEW_STATIC
#include <GL/glew.h>

// Library for window creation and event handling
#include <GLFW/glfw3.h>

// Library for vertex and matrix math
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Library for loading .OBJ model
#define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>

#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include <Eigen/Sparse>
#include <Eigen/Dense>

#include "lodepng.h"
#include "visual studio\ImageExporter.h"
#include "visual studio\ShaderLoader.h"
#include "visual studio\Drawer.h"
#include "visual studio\Vertex.h"
#include "visual studio\OptixPrimeFunctionality.h"
#include "visual studio\Defines.h"
#include "visual studio\InputHandler.h"
#include "visual studio\INIReader.h"
#include "visual studio\Camera.h"
#include "visual studio\MeshS.h"
#include "visual studio\Lightning.h"

typedef Eigen::SparseMatrix<float> SpMat;

// optixview is an array containing all pixel values
std::vector<glm::vec3> optixView;
std::vector<std::vector<MatrixIndex>> trianglesonScreen;
std::vector<optix_functionality::Hit> patches;

Drawer::DebugLine debugline;
GLuint optixTex, optixVao;

int main() {
	// print header
	std::ifstream f("print_header.txt");
	if (f.is_open())
		std::cout << f.rdbuf();
	f.close();

	// load config
	INIReader reader("config.ini");
	if (reader.ParseError() != 0) {
		std::cout << "Can't load 'config.ini'\n";
		return 1;
	}
	int WIDTH = reader.GetInteger("window", "width", -1);
	int HEIGHT = reader.GetInteger("window", "height", -1);
	float emission_value = reader.GetReal("lightning", "emission_value", -1);
	int method = reader.GetInteger("lightning", "method", 0);
	bool radiosityRendering = reader.GetBoolean("drawing", "radiosityRendering", false);
	bool antiAliasing = reader.GetBoolean("drawing", "antiAliasing", false);
	int supersampling = reader.GetInteger("drawing", "supersampling", 4);
	bool cuda_on = reader.GetBoolean("acceleration", "cuda_on", false);
	char * obj_filepath = new char[reader.Get("filepaths", "scene", "UNKNOWN").length() + 1];
	std::strcpy(obj_filepath, reader.Get("filepaths", "scene", "UNKNOWN").c_str());
	char * mtl_dirpath = new char[reader.Get("filepaths", "mtl_dir", "testscenes/").length() + 1];
	std::strcpy(mtl_dirpath, reader.Get("filepaths", "mtl_dir", "testscenes/").c_str());
	char * store_mat_filepath = new char[reader.Get("filepaths", "scene", "UNKNOWN").length() - 4];
	std::strncpy(store_mat_filepath, reader.Get("filepaths", "scene", "UNKNOWN").c_str(), reader.Get("filepaths", "scene", "UNKNOWN").length() - 4);
	store_mat_filepath[reader.Get("filepaths", "scene", "UNKNOWN").length() - 4] = 0;
	std::cout << "mat file path " << store_mat_filepath << std::endl;
	
	// set up camera
	Camera camera(WIDTH, HEIGHT, supersampling);

	//initialize window, GLEW extension loader & OpenGL debug callback
	GLFWwindow* window = Drawer::initWindow(WIDTH, HEIGHT);
	glewExperimental = GL_TRUE;
	glewInit();
	glDebugMessageCallback(Drawer::debugCallback, nullptr);

	std::vector<float> wavelengths = { 200.0, 250.0, 300.0, 350.0, 400.0, 450.0, 500.0, 550.0, 600.0 };

	// load scene into mesh & initialize optix
	MeshS mesh(obj_filepath, mtl_dirpath, wavelengths);
	OptixPrimeFunctionality optixP(mesh);

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	Drawer::debuglineInit(linevao, linevbo, debugprogram);

	// initialize radiosity matrix
	int numtriangles = mesh.triangleIndices.size();

	// set up lightning
	Lightning* lightning = Lightning::get_lightning(method, mesh, optixP, emission_value, wavelengths, cuda_on, store_mat_filepath);

	//initializing result optix drawing
	GLuint optixShader;
	Drawer::RenderContext rendercontext(trianglesonScreen, *lightning, optixView, mesh, camera, debugprogram, linevao, linevbo, radiosityRendering, antiAliasing, supersampling);
	optixP.traceScreen(rendercontext);
	Drawer::initRes(optixShader, optixVao, optixTex, WIDTH, HEIGHT, optixView);

	// set up callback context
	patches.resize(2);
	InputHandler inputhandler;
	callback_context cbc(debugline, patches, optixP, rendercontext, inputhandler);
	glfwSetWindowUserPointer(window, &cbc);

	// Some neat casting of member functions such that we can use them as callback AND have state too, as explained per:
	// https://stackoverflow.com/a/28660673/7925249
	auto func_key = [](GLFWwindow* window, int key, int scancode, int action, int mods) { static_cast<InputHandler*>(glfwGetWindowUserPointer(window))->key_callback(window, key, scancode, action, mods); };
	auto func_mouse = [](GLFWwindow* window, int button, int action, int mods) { static_cast<InputHandler*>(glfwGetWindowUserPointer(window))->mouse_button_callback(window, button, action, mods); };
	auto func_cursor = [](GLFWwindow* window, double xpos, double ypos) { static_cast<InputHandler*>(glfwGetWindowUserPointer(window))->cursor_pos_callback(window, xpos, ypos); };

	// Set up input callbacks
	glfwSetMouseButtonCallback(window, func_mouse);
	glfwSetCursorPosCallback(window, func_cursor);
	glfwSetKeyCallback(window, func_key);

	std::cout << "All is set up! Get ready to play around!" << std::endl;
	std::ifstream f_menu("print_menu.txt");
	if (f_menu.is_open())
		std::cout << f_menu.rdbuf();
	f_menu.close();

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		// glfwWaitEvents is preferred over glwfPollEvents for our application as we do not have to render in between input callbacks
		glfwWaitEvents();
		Drawer::draw(window, optixShader, optixVao, debugline, optixP, rendercontext);
	}

	// clean up
	glfwDestroyWindow(window);
	glfwTerminate();

	delete lightning;
	lightning = nullptr;

	return 0;
}