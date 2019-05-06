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

// Variables to be set from config.ini file
int WIDTH, HEIGHT, optixW, optixH;
char * obj_filepath;
int emission_index;
float emission_value;
bool radiosityRendering;

// The Matrix
typedef Eigen::SparseMatrix<float> SpMat;

std::vector<Vertex> debugline = { { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) } };

optix::float3 eye = optix::make_float3(0.0f, 0.0f, -7.0f);
optix::float3 viewDirection = optix::make_float3(0.0f, 0.0f, 1.0f);
optix::Context context;
OptixPrimeFunctionality optixP;
std::vector<Vertex> vertices;
// optixview is an array containing all pixel values
std::vector<glm::vec3> optixView;
std::vector<std::vector<MatrixIndex>> trianglesonScreen;
std::vector<optix_functionality::Hit> patches;
bool left = true; 
bool hitB = false;
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
	WIDTH = reader.GetInteger("window", "width", -1);
	HEIGHT = reader.GetInteger("window", "height", -1);
	optixW = reader.GetInteger("optix", "optixW", -1);
	optixH = reader.GetInteger("optix", "optixH", -1);
	obj_filepath = new char[reader.Get("filepaths", "scene", "UNKNOWN").length() + 1];
	std::strcpy(obj_filepath, reader.Get("filepaths", "scene", "UNKNOWN").c_str());
	emission_index = reader.GetInteger("lightning", "emission_index", -1);
	emission_value = reader.GetReal("lightning", "emission_value", -1);
	radiosityRendering = reader.GetBoolean("drawing", "radiosityRendering", false);
	
	//initialize window
	GLFWwindow* window = Drawer::initWindow(WIDTH, HEIGHT);

	// Initialize GLEW extension loader
	glewExperimental = GL_TRUE;
	glewInit();

	// load scene
	Vertex::loadVertices(vertices, obj_filepath);

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
	optixP = OptixPrimeFunctionality();
	optixP.initOptixPrime(vertices);

	//initializing result optix drawing
	GLuint optixShader;
	optixP.doOptixPrime(optixW, optixH, optixView, eye, viewDirection, trianglesonScreen, vertices);
	Drawer::initRes(optixShader, optixVao, optixTex, optixW, optixH, optixView);

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	Drawer::debuglineInit(linevao, linevbo, debugprogram);

	// initialize radiosity matrix
	int numtriangles = vertices.size() / 3;
	SpMat RadMat(numtriangles, numtriangles);
	optixP.calculateRadiosityMatrix(RadMat, vertices, rands);
	//// little debug output to check something happened while calculating the matrix:
	//std::cout << "total entries in matrix = " << numtriangles*numtriangles << std::endl;
	//std::cout << "non zeros in matrix = " << RadMat.nonZeros() << std::endl;
	//std::cout << "percentage non zero entries = " << (float(RadMat.nonZeros()) / float(numtriangles*numtriangles))*100 << std::endl;

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

	// Set up OpenGL debug callback
	glDebugMessageCallback(Drawer::debugCallback, nullptr);
	glfwSetMouseButtonCallback(window, InputHandler::mouse_button_callback);
	glfwSetKeyCallback(window, InputHandler::key_callback);

	// set up callback context
	patches.resize(2);
	InputHandler::callback_context cbc(left, hitB, debugline, optixW, optixH, viewDirection, eye, trianglesonScreen, optixView, patches, vertices, rands, optixP, lightningvalues, RadMat, emission, numpasses, residualvector, radiosityRendering);
	glfwSetWindowUserPointer(window, &cbc);

	// print menu
	std::ifstream f_menu("print_menu.txt");
	if (f_menu.is_open())
		std::cout << f_menu.rdbuf();
	f_menu.close();

	if (radiosityRendering){
		Drawer::setRadiosityTex(trianglesonScreen, lightningvalues, optixView, optixW, optixH);
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		Drawer::draw(window, optixShader, optixVao, debugprogram, linevao, linevbo, debugline, hitB);
	}

	// clean up
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}