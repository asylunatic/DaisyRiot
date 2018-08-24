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
#include <tiny_obj_loader.h>

// Library for loading an image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>

#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include "lodepng.h"
#include "visual studio\ImageExporter.h"
#include "visual studio\ShaderLoader.h"

// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;

// Per-vertex data
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
};

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
	for (size_t j = 0; j < optixH; j++) {
		for (size_t i = 0; i < optixW; i++) {
			rays[(j*optixH + i) * 2] = eye;
			rays[((j*optixH + i) * 2) + 1] = optix::normalize(upperLeftCorner + optix::make_float3(i*2.0f/optixW, -1.0f*(j*2.0f/optixH), 0) - eye);
		}

	}
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

void debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram) {
	// creating vao
	glGenVertexArrays(1, &linevao);
	// creating vertexbuffer for the vao
	glGenBuffers(1, &linevbo);

	GLuint vertexShader;
	ShaderLoader::loadShader(vertexShader, "shaders/debugshader.vert", GL_VERTEX_SHADER);
	GLuint fragmentShader;
	ShaderLoader::loadShader(fragmentShader, "shaders/debugshader.frag", GL_FRAGMENT_SHADER);

	// Combine vertex and fragment shaders into single shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
}

void debuglineDraw(GLuint &debugprogram, GLuint &linevao, GLuint &linevbo) {
	glUseProgram(debugprogram);
	glBindVertexArray(linevao);
	// vao will get info from linevbo now
	glBindBuffer(GL_ARRAY_BUFFER, linevbo);
	//insert data into the current array_buffer: the vbo
	glBufferData(GL_ARRAY_BUFFER, debugline.size()*sizeof(Vertex), debugline.data(), GL_STREAM_DRAW);
	// The position vectors should be retrieved from the specified Vertex Buffer Object with given offset and stride
	// Stride is the distance in bytes between vertices
	//INFO: glVertexAttribPointer always loads the data from GL_ARRAY_BUFFER, and puts it into the current VertexArray
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	GLuint hitLoc = glGetUniformLocation(debugprogram, "hit");
	glUniform1i(hitLoc, hitB);
	glDrawArrays(GL_LINES, 0, 2);
}

// OpenGL debug callback
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		std::cerr << "OpenGL: " << message << std::endl;
	}
}

void setResDrawing() {
	// Create Vertex Buffer Object
	GLuint vbo;
	glGenBuffers(1, &vbo);
	//bind it as (possible) source for the vao
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//insert data into the current array_buffer: the vbo
	std::vector<glm::vec3> quad = { glm::vec3(-1, 1, -1), glm::vec3(1, 1, -1), glm::vec3(-1, -1, -1), glm::vec3(1, -1, -1), glm::vec3(-1, -1, -1), glm::vec3(1, 1, -1) };
	glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(glm::vec3), quad.data(), GL_STATIC_DRAW);

	// Bind vertex data to shader inputs using their index (location)
	// These bindings are stored in the Vertex Array Object
	glGenVertexArrays(1, &optixVao);
	glBindVertexArray(optixVao);

	// The position vectors should be retrieved from the specified Vertex Buffer Object with given offset and stride
	// Stride is the distance in bytes between vertices
	//INFO: glVertexAttribPointer always loads the data from GL_ARRAY_BUFFER, and puts it into the VertexArray
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// Create Texture
	// everything will now be done in GL_TEXTURE1
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &optixTex);
	glBindTexture(GL_TEXTURE_2D, optixTex);

	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, optixW, optixH, 0, GL_RGB, GL_FLOAT, optixView.data());
}

void initRes(GLuint &shaderProgram) {
	GLuint vertexShader;
	ShaderLoader::loadShader(vertexShader, "shaders/optixShader.vert", GL_VERTEX_SHADER);

	GLuint fragmentShader;
	ShaderLoader::loadShader(fragmentShader, "shaders/optixShader.frag", GL_FRAGMENT_SHADER);

	// Combine vertex and fragment shaders into single shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	if (!ShaderLoader::checkProgramErrors(shaderProgram)) {
		std::cerr << "Program failed to link!" << std::endl;
		//return EXIT_FAILURE;
	}

	setResDrawing();

	// Set behaviour for when texture coordinates are outside the [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set interpolation for texture sampling (GL_NEAREST for no interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void drawRes(GLuint &shaderProgram, GLuint &vao) {
	// Bind the shader
	glUseProgram(shaderProgram);
	GLuint loc;

	// upload texture coordinates
	loc = glGetUniformLocation(shaderProgram, "texToon");
	glUniform1i(loc, 1);

	// Bind vertex data
	glBindVertexArray(vao);
	// Execute draw command
	glDrawArrays(GL_TRIANGLES, 0, 6);
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
		std::cout << "move left" << std::endl;
		//eye = eye - optix::make_float3(0.0005f, 0.0f, 0.0f);
		//viewDirection = viewDirection - optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		setResDrawing();
	}

	if (key == GLFW_KEY_RIGHT) {
		std::cout << "move right" << std::endl;
		eye = eye + optix::make_float3(0.0005f, 0.0f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		setResDrawing();
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
		//doOptix(xpos, ypos);
		//doOptixPrime(0, 0);
	}



}

int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW!" << std::endl;
		return EXIT_FAILURE;
	}

	// Create window and OpenGL 4.3 debug context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "X-Toon shader", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create OpenGL context!" << std::endl;
		return EXIT_FAILURE;
	}

	// Activate the OpenGL context
	glfwMakeContextCurrent(window);

	// Initialize GLEW extension loader
	glewExperimental = GL_TRUE;
	glewInit();

	// Set up OpenGL debug callback
	glDebugMessageCallback(debugCallback, nullptr);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);

	// Load vertices of model
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "balls.obj")) {
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	// Read triangle vertices from OBJ file
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			// Retrieve coordinates for vertex by index
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			// Retrieve components of normal by index
			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertices.push_back(vertex);
		}
	}
	//initializing optix
	initOptixPrime();

	//initializing result optix drawing
	GLuint optixShader;
	doOptixPrime();
	initRes(optixShader);
	std::cout << optixTex << std::endl;

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	debuglineInit(linevao, linevbo, debugprogram);

	patches.resize(2);

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Clear the framebuffer to black and depth to maximum value
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Drawing result from optix raytracing!
		drawRes(optixShader, optixVao);

		//DRAWING DEBUGLINE
		debuglineDraw(debugprogram, linevao, linevbo);

		// Present result to the screen
		glfwSwapBuffers(window);

	}

	// Cleanup
	//stbi_image_free(pixels);

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}