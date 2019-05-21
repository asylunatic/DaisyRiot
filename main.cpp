// Library for OpenGL function loading
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

// Variables to be set from config.ini file
int WIDTH, HEIGHT;
char * obj_filepath;
int emission_index;
float emission_value;
bool radiosityRendering;

// The Matrix
typedef Eigen::SparseMatrix<float> SpMat;

optix::Context context;
vertex::MeshS mesh = { std::vector<glm::vec3>(), 
					   std::vector<glm::vec3>(), 
					   std::vector<vertex::TriangleIndex>(), 
					   std::vector<std::vector<int>>()};
// optixview is an array containing all pixel values
std::vector<glm::vec3> optixView;
std::vector<std::vector<MatrixIndex>> trianglesonScreen;
std::vector<optix_functionality::Hit> patches;

Drawer::DebugLine debugline;
GLuint optixTex, optixVao;
std::vector<UV> rands;
Eigen::VectorXf lightningvalues;

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
	emission_index = reader.GetInteger("lightning", "emission_index", -1);
	emission_value = reader.GetReal("lightning", "emission_value", -1);
	radiosityRendering = reader.GetBoolean("drawing", "radiosityRendering", false);
	
	// set up camera
	Camera camera(WIDTH, HEIGHT);

	//initialize window
	GLFWwindow* window = Drawer::initWindow(WIDTH, HEIGHT);

	// Initialize GLEW extension loader
	glewExperimental = GL_TRUE;
	glewInit();

	// load scene
	vertex::loadVertices(mesh, obj_filepath);

	// set up randoms to be reused
	rands.resize(RAYS_PER_PATCH);
	std::srand(std::time(nullptr)); // use current time as seed for random generator
 	for (size_t i = 0; i < RAYS_PER_PATCH; i++) {
		UV uv = UV();
		uv.u = ((float)(rand() % RAND_MAX)) / RAND_MAX;
		uv.v = ((float)(rand() % RAND_MAX)) / RAND_MAX;
		uv.v = uv.v * (1 - uv.u);
		rands[i] = uv;
	}

	//initializing optix
	OptixPrimeFunctionality optixP(mesh);

	//initializing result optix drawing
	GLuint optixShader;
	optixP.traceScreen(optixView, camera, trianglesonScreen, mesh);
	Drawer::initRes(optixShader, optixVao, optixTex, WIDTH, HEIGHT, optixView);

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	Drawer::debuglineInit(linevao, linevbo, debugprogram);

	// initialize radiosity matrix
	int numtriangles = mesh.triangleIndices.size();
	SpMat RadMat(numtriangles, numtriangles);
	optixP.calculateRadiosityMatrixStochastic(RadMat, mesh, rands);

	// set up lightning
	Eigen::VectorXf emission = Eigen::VectorXf::Zero(numtriangles);
	emission(emission_index) = emission_value;	
	lightningvalues = Eigen::VectorXf::Zero(numtriangles);
	lightningvalues = emission; 
	Eigen::VectorXf residualvector = Eigen::VectorXf::Zero(numtriangles);
	residualvector = emission;
	int numpasses = 0;
	// converge lightning
	while (residualvector.sum() > 0.0001){
		residualvector = RadMat * residualvector;
		lightningvalues = lightningvalues + residualvector;
		numpasses++;
	}

	// set up callback context
	patches.resize(2);
	InputHandler inputhandler;
	callback_context cbc(debugline, camera, trianglesonScreen, optixView, patches, mesh, rands, optixP, lightningvalues, RadMat, emission, numpasses, residualvector, radiosityRendering, inputhandler);
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
		Drawer::setRadiosityTex(trianglesonScreen, lightningvalues, optixView, WIDTH, HEIGHT, mesh);
		Drawer::refreshTexture(WIDTH, HEIGHT, optixView);
	}

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		// glfwWaitEvents is preferred over glwfPollEvents as we do not have to render in between input callbacks
		//glfwPollEvents();
		glfwWaitEvents();
		Drawer::draw(window, optixShader, optixVao, debugprogram, linevao, linevbo, debugline);
	}

	// clean up
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}