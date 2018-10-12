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

// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;

const char * filepath = "balls.obj";

struct Hit {
	float t;
	int triangleId;
	optix::float2 uv;
};

struct MatrixIndex {
	int col;
	int row;
};

struct UV {
	float u;
	float v;
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
std::vector<UV> rands;
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
		origin = uv2xyz(originPatch, optix::make_float2(rands[i].u, rands[i].v));
		dest = uv2xyz(destPatch, optix::make_float2(rands[i].u, rands[i].v));

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
		origin = uv2xyz(originPatch, optix::make_float2(rands[i].u, rands[i].v));
		dest = uv2xyz(destPatch, optix::make_float2(rands[i].u, rands[i].v));
		rays[i * 2] = origin + optix::normalize(dest - origin)*0.000001f;
		rays[i * 2 + 1] = optix::normalize(dest - origin);
		//printf("\nuv = %f, %f");
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
		Drawer::refreshTexture(optixW,  optixH, optixView);
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		std::cout << "right" << std::endl;
		eye = eye + optix::make_float3(0.5f, 0.0f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		std::cout << "up" << std::endl;
		eye = eye + optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);

		doOptixPrime();
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		std::cout << "down" << std::endl;
		eye = eye - optix::make_float3(0.0f, 0.5f, 0.0f);
		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
		doOptixPrime();
		Drawer::refreshTexture(optixW, optixH, optixView);
	}

	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		float formfactor = p2pFormfactor2(patches[0].triangleId, patches[1].triangleId);
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