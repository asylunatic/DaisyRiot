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
#include "lodepng.h"
#include "visual studio\ImageExporter.h"
#include "visual studio\ShaderLoader.h"
#include "visual studio\Drawer.h"
#include "visual studio\Vertex.h"
#include "visual studio\OptixPrimeFunctionality.h"
#include "visual studio\Defines.h"
#include "visual studio\InputHandler.h"

// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;

const char * obj_filepath = "balls.obj";

std::vector<Vertex> debugline = { { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) } };

optix::float3 eye = optix::make_float3(0.0f, 0.0f, -7.0f);
optix::float3 viewDirection = optix::make_float3(0.0f, 0.0f, 1.0f);
optix::Context context;
optix::prime::Context contextP;
optix::prime::Model model;
std::vector<Vertex> vertices;
std::vector<glm::vec3> optixView;
std::vector<std::vector<MatrixIndex>> trianglesonScreen;
std::vector<OptixFunctionality::Hit> patches;
bool left = true; 
int optixW = 512, optixH = 512;
bool hitB = false;
GLuint optixTex, optixVao;
std::vector<UV> rands;

// key button callback to print screen
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (key == GLFW_KEY_P && action == GLFW_PRESS){
		std::cout << "print" << std::endl;
		// write png image
		ImageExporter ie = ImageExporter(WIDTH, HEIGHT);
		glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, ie.encodepixels);
		if (GL_NO_ERROR != glGetError()) throw "Error: Unable to read pixels.";
		ie.encodeOneStep("screenshots/output",".png", WIDTH, HEIGHT);
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		std::cout << "left" << std::endl;
		eye = eye - optix::make_float3(0.5f, 0.0f, 0.0f);
		viewDirection = viewDirection - optix::make_float3(0.0005f, 0.0f, 0.0f);
		OptixPrimeFunctionality::doOptixPrime(optixW, optixH, contextP, optixView, eye, viewDirection, model, trianglesonScreen, vertices);
		Drawer::refreshTexture(optixW,  optixH, optixView);
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		std::cout << "right" << std::endl;
		eye = eye + optix::make_float3(0.5f, 0.0f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		OptixPrimeFunctionality::doOptixPrime(optixW, optixH, contextP, optixView, eye, viewDirection, model, trianglesonScreen, vertices);
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		std::cout << "up" << std::endl;
		eye = eye + optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);

		OptixPrimeFunctionality::doOptixPrime(optixW, optixH, contextP, optixView, eye, viewDirection, model, trianglesonScreen, vertices);
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		std::cout << "down" << std::endl;
		eye = eye - optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		OptixPrimeFunctionality::doOptixPrime(optixW, optixH, contextP, optixView, eye, viewDirection, model, trianglesonScreen, vertices);
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		float formfactor = OptixPrimeFunctionality::p2pFormfactor2(patches[0].triangleId, patches[1].triangleId, vertices, contextP, model, rands);
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		int id;
		std::cout << "\nenter the id of the triangle you want to find" << std::endl;
		std::cin >> id;
		for (MatrixIndex index : trianglesonScreen[id]) {
			optixView[(index.row*optixH + index.col)] = glm::vec3(0.0, 1.0, 0.0);
		}
		Drawer::refreshTexture(optixW, optixH, optixView);
	}
}

int main() {
	
	//initialize window
	GLFWwindow* window = Drawer::initWindow(WIDTH, HEIGHT);

	// Initialize GLEW extension loader
	glewExperimental = GL_TRUE;
	glewInit();

	// Set up OpenGL debug callback
	glDebugMessageCallback(Drawer::debugCallback, nullptr);
	glfwSetMouseButtonCallback(window, InputHandler::mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);
	// set up callback context
	InputHandler::callback_context cbc(left, hitB,debugline, optixW, optixH,viewDirection, eye,  trianglesonScreen, model, optixView, patches, vertices);
	glfwSetWindowUserPointer(window, &cbc);

	Vertex::loadVertices(vertices, obj_filepath);

	rands.resize(RAYS_PER_PATCH);
	std::srand(std::time(nullptr)); // use current time as seed for random generator
	printf("\nrands: ");
 	for (size_t i = 0; i < RAYS_PER_PATCH; i++) {
		UV uv = UV();
		uv.u = ((float)(rand() % RAND_MAX)) / RAND_MAX;
		uv.v = ((float)(rand() % RAND_MAX)) / RAND_MAX;
		uv.v = uv.v * (1 - uv.u);
		printf("\nsum uv : %f", uv.v + uv.u);
		rands[i] = uv;
	}

	//initializing optix
	OptixPrimeFunctionality::initOptixPrime(contextP, model, vertices);

	//initializing result optix drawing
	GLuint optixShader;
	OptixPrimeFunctionality::doOptixPrime(optixW, optixH, contextP, optixView, eye, viewDirection, model,  trianglesonScreen, vertices);
	Drawer::initRes(optixShader, optixVao, optixTex, optixW, optixH, optixView);
	std::cout << optixTex << std::endl;

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	Drawer::debuglineInit(linevao, linevbo, debugprogram);

	patches.resize(2);

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