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

// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;

const char * filepath = "balls.obj";

struct Hit {
	float t;
	int triangleId;
	optix::float2 uv;
};

std::vector<Vertex> debugline = { { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) } };

optix::float3 eye = optix::make_float3(0.0f, 0.0f, -7.0f);
optix::float3 viewDirection = optix::make_float3(0.0f, 0.0f, 1.0f);
optix::Context context;
optix::prime::Context contextP;
optix::prime::Model model;
std::vector<Vertex> vertices;
std::vector<glm::vec3> optixView;
std::vector<Hit> patches;
bool left = true; 
int optixW = 512, optixH = 512;
bool hitB = false;
GLuint optixTex, optixVao;

optix::float3 uv2xyz(int triangleId, optix::float2 uv) {
	glm::vec3 a = vertices[triangleId * 3].pos;
	glm::vec3 b = vertices[triangleId * 3 + 1].pos;
	glm::vec3 c = vertices[triangleId * 3 + 2].pos;
	glm::vec3 point = glm::vec3(a + uv.x*(b - a) + uv.y*(c - a));
	return optix::make_float3(point.x, point.y, point.z);
}

void initOptix() {

	/* *******************************************************OPTIX******************************************************* */
	//initializing context -> holds all programs and variables
	context = optix::Context::create();

	//enable printf shit
	context->setPrintEnabled(true);
	context->setPrintBufferSize(4096);

	//setting entry point: a ray generationprogram
	context->setEntryPointCount(1);
	optix::Program origin = context->createProgramFromPTXFile("ptx\\FirstTry.ptx", "raytraceExecution");
	origin->validate();
	context->setRayGenerationProgram(0, origin);


	//initialising buffers and loading normals into buffer
	RTbuffer buffer;
	rtBufferCreate(context->get(), RT_BUFFER_INPUT, &buffer);
	rtBufferSetFormat(buffer, RT_FORMAT_USER);
	rtBufferSetElementSize(buffer, sizeof(glm::vec3));
	rtBufferSetSize1D(buffer, vertices.size());
	void* data;
	rtBufferMap(buffer, &data);
	glm::vec3* vertex_data = (glm::vec3*) data;
	for (int i = 0; i < vertices.size(); i++) {
		vertex_data[i] = vertices[i].normal;
	}
	rtBufferUnmap(buffer);

	optix::Geometry geometryH = context->createGeometry();
	optix::Material materialH = context->createMaterial();
	optix::GeometryInstance geometryInstanceH = context->createGeometryInstance();
	optix::GeometryGroup geometryGroupH = context->createGeometryGroup();

	optix::Program intersection;
	try {
		intersection = context->createProgramFromPTXFile("ptx\\geometry.ptx", "intersection");
	}
	catch (optix::Exception e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	intersection->validate();

	optix::Program boundingbox = context->createProgramFromPTXFile("ptx\\geometry.ptx", "boundingbox");
	boundingbox->validate();

	geometryH->setPrimitiveCount(1);
	geometryH->setIntersectionProgram(intersection);
	geometryH->setBoundingBoxProgram(boundingbox);

	geometryInstanceH->setMaterialCount(1);
	geometryInstanceH->setGeometry(geometryH);
	geometryInstanceH->setMaterial(0, materialH);

	geometryGroupH->setChildCount(1);
	geometryGroupH->setChild(0, geometryInstanceH);
	geometryGroupH->setAcceleration(context->createAcceleration("NoAccel"));

	context["top_object"]->set(geometryGroupH);

	printf("check");
	/* *******************************************************OPTIX******************************************************* */

}

void doOptix(double xpos, double ypos) {

	context["mousePos"]->setFloat((float)xpos, (float)ypos);

	context->launch(0, 1);
}

void initOptixPrime() {
	contextP = optix::prime::Context::create(RTP_CONTEXT_TYPE_CUDA);
	optix::prime::BufferDesc buffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_VERTEX_FLOAT3, RTP_BUFFER_TYPE_HOST, vertices.data());
	buffer->setRange(0, vertices.size());
	buffer->setStride(sizeof(Vertex));
	model = contextP->createModel();
	model->setTriangles(buffer);
	try{
		model->update(RTP_MODEL_HINT_NONE);
		model->finish();
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
}

void doOptixPrime() {

	std::vector<Hit> hits;
	hits.resize(optixW*optixH);
	optixView.resize(optixW*optixH);
	optix::float3 upperLeftCorner = eye + viewDirection + optix::make_float3(-1.0f, 1.0f, 0.0f);
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> rays;
	rays.resize(optixW*optixH * 2);

	//timing
	std::clock_t start;
	double duration;
	start = std::clock();
	for (size_t j = 0; j < optixH; j++) {
		for (size_t i = 0; i < optixW; i++) {
			rays[(j*optixH + i) * 2] = eye;
			rays[((j*optixH + i) * 2) + 1] = optix::normalize(upperLeftCorner + optix::make_float3(i*2.0f/optixW, -1.0f*(j*2.0f/optixH), 0) - eye);
		}

	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "for looping: " << duration << '\n';
	start = std::clock();

	query->setRays(optixW*optixH, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
	hitBuffer->setRange(0, optixW*optixH);
	query->setHits(hitBuffer);

	try{
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	query->finish();


	for (size_t j = 0; j < optixH; j++) {
		for (size_t i = 0; i < optixW; i++) {
			optixView[(j*optixH + i)] = (hits[(j*optixH + i)].t>0) ? glm::vec3(glm::abs(vertices[hits[(j*optixH + i)].triangleId * 3].normal)) : glm::vec3(0.0f, 0.0f, 0.0f);
		}

	}
}

bool shootPatchRay() {
	optix::float3  pointA = uv2xyz(patches[0].triangleId, patches[0].uv);
	optix::float3  pointB = uv2xyz(patches[1].triangleId, patches[1].uv);
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> ray = { pointA + optix::normalize(pointB - pointA)*0.000001f, optix::normalize(pointB - pointA) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	Hit hit;
	query->setHits(1, RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, &hit);
	try{
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	printf("hit t value: %f", hit.t);
	printf("\nclosest hit was: %i", hit.triangleId);
	printf("\npatch 1 was: %i", patches[0].triangleId);
	printf("\npatch 2 was: %i", patches[1].triangleId);
	if (hit.triangleId == patches[1].triangleId) {
		return true;
	}
	else return false;
}

void intersectMouse(double xpos, double ypos) {
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> ray = { eye, optix::normalize(optix::make_float3(xpos + viewDirection.x, ypos + viewDirection.y, viewDirection.z)) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	Hit hit;
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
			hitB = shootPatchRay();
			printf("\ndid it hit? %i", hitB);
		}
		left = !left;

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
	if (key == GLFW_KEY_LEFT) {
		eye = eye - optix::make_float3(0.5f, 0.0f, 0.0f);
		viewDirection = viewDirection - optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		Drawer::refreshTexture(optixW,  optixH, optixView);
	}

	if (key == GLFW_KEY_RIGHT) {
		eye = eye + optix::make_float3(0.5f, 0.0f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	if (key == GLFW_KEY_UP) {
		eye = eye + optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);

		doOptixPrime();

		Drawer::refreshTexture(optixW, optixH, optixView);

	}

	if (key == GLFW_KEY_DOWN) {
		eye = eye - optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
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
		printf("click!: %f %f", xpos, ypos);
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

	//initializing optix
	initOptixPrime();

	//initializing result optix drawing
	GLuint optixShader;
	doOptixPrime();
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