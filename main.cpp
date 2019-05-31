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

// Library for loading an image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

// Variables to be set from config.ini file
int WIDTH, HEIGHT;
char * obj_filepath;
char * mtl_dirpath;
bool radiosityRendering;

// The Matrix
typedef Eigen::SparseMatrix<float> SpMat;

optix::Context context;
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
	// set variables from config
	WIDTH = reader.GetInteger("window", "width", -1);
	HEIGHT = reader.GetInteger("window", "height", -1);
	obj_filepath = new char[reader.Get("filepaths", "scene", "UNKNOWN").length() + 1];
	std::strcpy(obj_filepath, reader.Get("filepaths", "scene", "UNKNOWN").c_str());
	mtl_dirpath = new char[reader.Get("filepaths", "mtl_dir", "testscenes/").length() + 1];
	std::strcpy(mtl_dirpath, reader.Get("filepaths", "mtl_dir", "testscenes/").c_str());
	float emission_value = reader.GetReal("lightning", "emission_value", -1);
	radiosityRendering = reader.GetBoolean("drawing", "radiosityRendering", false);
	
	// set up camera
	Camera camera(WIDTH, HEIGHT);

	//initialize window
	GLFWwindow* window = Drawer::initWindow(WIDTH, HEIGHT);

	// Initialize GLEW extension loader
	glewExperimental = GL_TRUE;
	glewInit();

	// load scene
	MeshS mesh;
	mesh.loadFromFile(obj_filepath, mtl_dirpath);

	//initializing optix
	OptixPrimeFunctionality optixP(mesh);

	//initializing result optix drawing
	GLuint optixShader;
	optixP.traceScreen(optixView, camera, trianglesonScreen, mesh);
	Drawer::initRes(optixShader, optixVao, optixTex, WIDTH, HEIGHT, optixView);

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	Drawer::debuglineInit(linevao, linevbo, debugprogram);

	// set up lightning
	Lightning lightning(mesh, optixP, emission_value);

	// set up callback context
	patches.resize(2);
	InputHandler inputhandler;
	callback_context cbc(debugline, camera, trianglesonScreen, optixView, patches, mesh, optixP, lightning, radiosityRendering, inputhandler);
	glfwSetWindowUserPointer(window, &cbc);

	// Some neat casting of member functions such we can use them as callback AND have state too, as explained per:
	// https://stackoverflow.com/a/28660673/7925249
	auto func_key = [](GLFWwindow* window, int key, int scancode, int action, int mods) { static_cast<InputHandler*>(glfwGetWindowUserPointer(window))->key_callback(window, key, scancode, action, mods); };
	auto func_mouse = [](GLFWwindow* window, int button, int action, int mods) { static_cast<InputHandler*>(glfwGetWindowUserPointer(window))->mouse_button_callback(window, button, action, mods); };
	auto func_cursor = [](GLFWwindow* window, double xpos, double ypos) { static_cast<InputHandler*>(glfwGetWindowUserPointer(window))->cursor_pos_callback(window, xpos, ypos); };

	// Set up OpenGL debug callback
	glDebugMessageCallback(Drawer::debugCallback, nullptr);
	// Set up input callbacks
	glfwSetMouseButtonCallback(window, func_mouse);
	glfwSetCursorPosCallback(window, func_cursor);
	glfwSetKeyCallback(window, func_key);

	std::cout << "All is set up! Get ready to play around!" << std::endl;
	// print menu
	std::ifstream f_menu("print_menu.txt");
	if (f_menu.is_open())
		std::cout << f_menu.rdbuf();
	f_menu.close();

	if (radiosityRendering){
		Drawer::setRadiosityTex(trianglesonScreen, lightning.lightningvalues, optixView, WIDTH, HEIGHT, mesh);
		Drawer::refreshTexture(WIDTH, HEIGHT, optixView);
	}

	Drawer::RenderContext rendercontext(trianglesonScreen, lightning.lightningvalues, optixView, mesh, camera, debugprogram, linevao, linevbo, radiosityRendering);

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		// glfwWaitEvents is preferred over glwfPollEvents for our application as we do not have to render in between input callbacks
		glfwWaitEvents();
		Drawer::draw(window, optixShader, optixVao, debugline, optixP, rendercontext);
	}

	// clean up
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}