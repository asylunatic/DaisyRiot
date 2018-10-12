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
#include <tiny_obj_loader.h>

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

struct MatrixIndex {
	int col;
	int row;
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
std::vector<std::vector<MatrixIndex>> trianglesonScreen;
std::vector<Hit> patches;
bool left = true; 
int optixW = 512, optixH = 512;
bool hitB = false;
GLuint optixTex, optixVao;
std::vector<float> rands;
int raysPerPatch = 100;

glm::vec3 optix2glmf3(optix::float3 v) {
	return glm::vec3(v.x, v.y, v.z);
}

optix::float3 glm2optixf3(glm::vec3 v) {
	return optix::make_float3(v.x, v.y, v.z);
}

optix::float3 uv2xyz(int triangleId, optix::float2 uv) {
	glm::vec3 a = vertices[triangleId * 3].pos;
	glm::vec3 b = vertices[triangleId * 3 + 1].pos;
	glm::vec3 c = vertices[triangleId * 3 + 2].pos;
	glm::vec3 point = a  + uv.x*(b - a) + uv.y*(c - a);
	return optix::make_float3(point.x, point.y, point.z);
}

void refreshTexture() {
	glActiveTexture(GL_TEXTURE1);
	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, optixW, optixH, 0, GL_RGB, GL_FLOAT, optixView.data());
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

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "optix rayshooting " << duration << '\n';
	start = std::clock();

	trianglesonScreen.clear();
	trianglesonScreen.resize(vertices.size()/3);

	for (int j = 0; j < optixH; j++) {
		for (int i = 0; i < optixW; i++) {
			optixView[(j*optixH + i)] = (hits[(j*optixH + i)].t>0) ? glm::vec3(glm::abs(vertices[hits[(j*optixH + i)].triangleId * 3].normal)) : glm::vec3(0.0f, 0.0f, 0.0f);
			if (hits[(j*optixH + i)].t>0) {
				MatrixIndex index = {};
				index.col = i;
				index.row = j;
				trianglesonScreen[hits[(j*optixH + i)].triangleId].push_back(index);
			}
		}

	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "for looping again: " << duration << '\n';
	start = std::clock();
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
	}/*
	printf("hit t value: %f", hit.t);
	printf("\nclosest hit was: %i", hit.triangleId);
	printf("\npatch 1 was: %i", patches[0].triangleId);
	printf("\npatch 2 was: %i", patches[1].triangleId);*/
	if (hit.triangleId == patches[1].triangleId) {
		return true;
	}
	else return false;
}

glm::vec3 calculateCentre(float triangleId) {
	glm::vec3 centre = (vertices[triangleId * 3].pos + vertices[triangleId * 3 + 1].pos + vertices[triangleId * 3 + 2].pos);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

float calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	float theta = glm::acos(glm::dot(ab, ac) / (glm::length(ab)*glm::length(ac)));
	float surface = 0.5 * glm::length(ab)*glm::length(ac) + glm::sin(theta);
	printf("\nsurface area is: %f", surface);
	return surface;
}

float p2pFormfactor(int originPatch, int destPatch) {

	glm::vec3 centreOrig = calculateCentre(originPatch);
	glm::vec3 centreDest = calculateCentre(destPatch);

	float formfactor = 0;
	float dot1 = glm::dot(vertices[originPatch * 3].normal, glm::normalize(centreDest - centreOrig));
	float dot2 = glm::dot(vertices[destPatch * 3].normal, glm::normalize(centreOrig - centreDest));
	if (dot1 > 0 && dot2 > 0) {
		float length = glm::sqrt(glm::dot(centreDest - centreOrig, centreDest - centreOrig));
		float theta1 = glm::acos(dot1 / length) * 180.0 / M_PIf;
		float theta2 = glm::acos(dot2 / length) * 180.0 / M_PIf;
		printf("\n theta's: %f, %f \nlength: %f \ndots: %f, %f", theta1, theta2, length, dot1, dot2);
		formfactor = dot1*dot2 / std::powf(length, 4)*M_PIf;
	}

	std::vector<optix::float3> rays;
	rays.resize(2 * raysPerPatch);

	std::vector<Hit> hits;
	hits.resize(raysPerPatch);

	optix::float3 origin;
	optix::float3 dest;
	for (int i = 0; i < raysPerPatch; i++) {
		origin = uv2xyz(originPatch, optix::make_float2(rands[i * 2], rands[i * 2 + 1]));
		dest = uv2xyz(destPatch, optix::make_float2(rands[i * 2], rands[i * 2 + 1]));

		rays[i * 2] = origin + optix::normalize(dest - origin)*0.000001f;
		rays[i * 2 + 1] = optix::normalize(dest - origin);
	}

	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	query->setRays(raysPerPatch, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
	hitBuffer->setRange(0, raysPerPatch);
	query->setHits(hitBuffer);
	try{
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}

	float visibility = 0;

	for (Hit hit : hits) {
		visibility += hit.t > 0 ? 1 : 0;
	}
	visibility = visibility / raysPerPatch;
	printf("\nformfactor: %f \nvisibility: %f\%", formfactor, visibility);
	return formfactor*visibility;

}

float p2pFormfactor2(int originPatch, int destPatch) {
	glm::vec3 centreOrig = calculateCentre(originPatch);
	//    A___B<------centreOrig
	//     \ /    ----/
	//      C<---/
	
	std::vector<glm::vec3> hemitriangle;
	std::vector<glm::vec3> projtriangle;

	projtriangle.resize(3);
	hemitriangle.resize(3);

	for (int i = 0; i < 2; i++) {
		hemitriangle[i] = glm::normalize(vertices[destPatch * 3 + i].pos - centreOrig);
		projtriangle[i] = hemitriangle[i] - glm::dot(vertices[originPatch * 3].normal, hemitriangle[i])*vertices[originPatch * 3].normal;
	}

	std::vector<optix::float3> rays;
	rays.resize(2 * raysPerPatch);

	std::vector<Hit> hits;
	hits.resize(raysPerPatch);

	optix::float3 origin;
	optix::float3 dest;
	for (int i = 0; i < raysPerPatch; i++) {
		origin = uv2xyz(originPatch, optix::make_float2(rands[i * 2], rands[i * 2 + 1]));
		dest = uv2xyz(destPatch, optix::make_float2(rands[i * 2], rands[i * 2 + 1]));
		printf("\n%f", rands[i*2]);
		rays[i * 2] = origin + optix::normalize(dest - origin)*0.000001f;
		rays[i * 2 + 1] = optix::normalize(dest - origin);
	}

	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	query->setRays(raysPerPatch, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
	hitBuffer->setRange(0, raysPerPatch);
	query->setHits(hitBuffer);
	try{
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}

	float visibility = 0;


	for (Hit hit : hits) {
		printf("\n%f", hit.t);
		float newT = hit.t > 0 && hit.triangleId == destPatch ? 1 : 0;
		visibility += newT;
		printf(" newT: %f triangle: %i", newT, hit.triangleId);
	}
	printf("rays hit: %f", visibility);
	visibility = visibility / raysPerPatch;
	float formfactor = calculateSurface(projtriangle[0], projtriangle[1], projtriangle[2]) / M_PIf;
	printf("\nformfactor: %f \nvisibility: %f", formfactor, visibility);

	return formfactor*visibility;

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
		for (MatrixIndex index : trianglesonScreen[hit.triangleId]) {
			optixView[(index.row*optixH + index.col)] = glm::vec3(1.0, 1.0, 1.0);
		}
		refreshTexture();

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
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		std::cout << "left" << std::endl;
		eye = eye - optix::make_float3(0.5f, 0.0f, 0.0f);
		viewDirection = viewDirection - optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		refreshTexture();
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		std::cout << "right" << std::endl;
		eye = eye + optix::make_float3(0.5f, 0.0f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		refreshTexture();
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		std::cout << "up" << std::endl;
		eye = eye + optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);

		doOptixPrime();
		refreshTexture();

	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		std::cout << "down" << std::endl;
		eye = eye - optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		refreshTexture();
	}

	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		if (shootPatchRay()) {
			float formfactor = p2pFormfactor2(patches[0].triangleId, patches[1].triangleId);
		}
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		int id;
		std::cout << "\nenter the id of the triangle you want to find" << std::endl;
		std::cin >> id;
		for (MatrixIndex index : trianglesonScreen[id]) {
			optixView[(index.row*optixH + index.col)] = glm::vec3(0.0, 1.0, 0.0);
		}
		refreshTexture();

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

	rands.resize(2*raysPerPatch);
	std::srand(std::time(nullptr)); // use current time as seed for random generator
	printf("\nrands: ");
 	for (size_t i = 0; i < 2*raysPerPatch; i++) {
		rands[i] = ((float) (rand() % RAND_MAX)) / RAND_MAX;
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