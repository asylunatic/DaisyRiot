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

// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;

const char * filepath = "balls.obj";

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
int raysPerPatch = RAYS_PER_PATCH;

void intersectMouse(double xpos, double ypos) {
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> ray = { eye, optix::normalize(optix::make_float3(xpos + viewDirection.x, ypos + viewDirection.y, viewDirection.z)) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	OptixFunctionality::Hit hit;
	query->setHits(1, RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, &hit);
	try{
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	query->finish();
	if (hit.t > 0) { 
		printf("\nhit triangle: %i ", hit.triangleId);
		if (left) patches[0] = hit;
		else {
			patches[1] = hit;
			printf("\nshoot ray between patches \n");
			printf("patch triangle 1: %i \n", patches[0].triangleId);
			printf("patch triangle 2: %i \n", patches[1].triangleId);
			hitB = OptixPrimeFunctionality::shootPatchRay(patches, vertices, model);
			printf("\ndid it hit? %i", hitB);
		}
		left = !left;
		for (MatrixIndex index : trianglesonScreen[hit.triangleId]) {
			optixView[(index.row*optixH + index.col)] = glm::vec3(1.0, 1.0, 1.0);
		}
		Drawer::refreshTexture(optixW, optixH, optixView);

	}
	else {
		printf("miss!");
		hitB = false;
		patches.clear();
		patches.resize(2);
		left = true; 
	}

}

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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		xpos = xpos * 2 / WIDTH - 1;
		ypos = 1 - ypos * 2 / HEIGHT;
		printf("\nclick!: %f %f", xpos, ypos);
		if (left){
			debugline.at(0) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
			}
		else {
			debugline.at(1) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
		}
		intersectMouse(xpos, ypos);
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
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);

	Vertex::loadVertices(vertices, filepath);

	rands.resize(raysPerPatch);
	std::srand(std::time(nullptr)); // use current time as seed for random generator
	printf("\nrands: ");
 	for (size_t i = 0; i < raysPerPatch; i++) {
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