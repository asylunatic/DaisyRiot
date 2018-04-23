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
#include <sys/stat.h>

#include <OptiX_world.h>
#include <optix_prime\optix_prime.h>
#include <optix_prime\optix_prime_declarations.h>
#include <optix_prime\optix_primepp.h>
#include "lodepng.h"


// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;

// Per-vertex data
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
};
std::vector<Vertex> debugline = { { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) } };

RTcontext context;
optix::Context contextH;
optix::prime::Context contextP;
optix::prime::Model model;
std::vector<float> hits;
bool left = true; 
int optixW = 512, optixH = 512;

glm::vec3 rgb2hsv(glm::vec3 in)
{
	glm::vec3         out;
	double      min, max, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min  < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max > in.b ? max : in.b;

	out.z = max;                                // v
	delta = max - min;
	if (delta < 0.00001)
	{
		out.y = 0;
		out.x = 0; // undefined, maybe nan?
		return out;
	}
	if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
		out.y = (delta / max);                  // s
	}
	else {
		// if max is 0, then r = g = b = 0              
		// s = 0, h is undefined
		out.y = 0.0;
		out.x = NAN;                            // its now undefined
		return out;
	}
	if (in.r >= max)                           // > is bogus, just keeps compilor happy
		out.x = (in.g - in.b) / delta;        // between yellow & magenta
	else
		if (in.g >= max)
			out.x = 2.0 + (in.b - in.r) / delta;  // between cyan & yellow
		else
			out.x = 4.0 + (in.r - in.g) / delta;  // between magenta & cyan

	out.x *= 60.0;                              // degrees

	if (out.x < 0.0)
		out.x += 360.0;

	return out;
}

glm::vec3 hsv2rgb(glm::vec3 in)
{
	double      hh, p, q, t, ff;
	long        i;
	glm::vec3   out;

	if (in.s <= 0.0) {       // < is bogus, just shuts up warnings
		out.r = in.z;
		out.g = in.z;
		out.b = in.z;
		return out;
	}
	hh = in.x;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.z * (1.0 - in.y);
	q = in.z * (1.0 - (in.y * ff));
	t = in.z * (1.0 - (in.y * (1.0 - ff)));

	switch (i) {
	case 0:
		out.r = in.z;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.z;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.z;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.z;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.z;
		break;
	case 5:
	default:
		out.r = in.z;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}

// Helper function to read a file like a shader
std::string readFile(const std::string& path) {
	std::ifstream file(path, std::ios::binary);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string R = buffer.str();
	return R;
}

bool checkShaderErrors(GLuint shader) {
	// Check if the shader compiled successfully
	GLint compileSuccessful;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccessful);

	// If it didn't, then read and print the compile log
	if (!compileSuccessful) {
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

		std::vector<GLchar> logBuffer(logLength);
		glGetShaderInfoLog(shader, logLength, nullptr, logBuffer.data());

		std::cerr << logBuffer.data() << std::endl;

		return false;
	}
	else {
		return true;
	}
}

bool checkProgramErrors(GLuint program) {
	// Check if the program linked successfully
	GLint linkSuccessful;
	glGetProgramiv(program, GL_LINK_STATUS, &linkSuccessful);

	// If it didn't, then read and print the link log
	if (!linkSuccessful) {
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

		std::vector<GLchar> logBuffer(logLength);
		glGetProgramInfoLog(program, logLength, nullptr, logBuffer.data());

		std::cerr << logBuffer.data() << std::endl;

		return false;
	}
	else {
		return true;
	}
}

void loadShader(GLuint &shader, std::string name, GLenum shaderType) {
	// Load and compile shader
	std::string shaderCode = readFile(name);
	const char* shaderCodePtr = shaderCode.data();

	shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderCodePtr, nullptr);
	glCompileShader(shader);

	if (!checkShaderErrors(shader)) {
		std::cerr << "Shader(s) failed to compile!" << std::endl;
		//return EXIT_FAILURE;
	}
}

void initOptix(std::vector<Vertex> &vertices) {

	/* *******************************************************OPTIX******************************************************* */
	//initializing context -> holds all programs and variables
	contextH = optix::Context::create();
	context = contextH->get();

	//enable printf shit
	rtContextSetPrintEnabled(context, 1);
	rtContextSetPrintBufferSize(context, 4096);

	//setting entry point: a ray generationprogram
	rtContextSetEntryPointCount(context, 1);
	RTprogram origin;
	rtProgramCreateFromPTXFile(context, "ptx\\FirstTry.ptx", "raytraceExecution", &origin);
	if (rtProgramValidate(origin) != RT_SUCCESS) {
		printf("Program is not complete.");
	}
	rtContextSetRayGenerationProgram(context, 0, origin);


	//initialising buffers and loading normals into buffer
	RTbuffer buffer;
	rtBufferCreate(context, RT_BUFFER_INPUT, &buffer);
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

	optix::Geometry geometryH = contextH->createGeometry();
	optix::Material materialH = contextH->createMaterial();
	optix::GeometryInstance geometryInstanceH = contextH->createGeometryInstance();
	optix::GeometryGroup geometryGroupH = contextH->createGeometryGroup();

	optix::Program intersection;
	try {
		intersection = contextH->createProgramFromPTXFile("ptx\\geometry.ptx", "intersection");
	}
	catch (optix::Exception e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	intersection->validate();

	optix::Program boundingbox = contextH->createProgramFromPTXFile("ptx\\geometry.ptx", "boundingbox");
	boundingbox->validate();

	geometryH->setPrimitiveCount(1);
	geometryH->setIntersectionProgram(intersection);
	geometryH->setBoundingBoxProgram(boundingbox);

	geometryInstanceH->setMaterialCount(1);
	geometryInstanceH->setGeometry(geometryH);
	geometryInstanceH->setMaterial(0, materialH);

	geometryGroupH->setChildCount(1);
	geometryGroupH->setChild(0, geometryInstanceH);
	geometryGroupH->setAcceleration(contextH->createAcceleration("NoAccel"));

	contextH["top_object"]->set(geometryGroupH);

	printf("check");
	/* *******************************************************OPTIX******************************************************* */

}

void doOptix(double xpos, double ypos) {

	contextH["mousePos"]->setFloat((float)xpos, (float)ypos);

	//rtContextLaunch1D(context, 0, 1);
	contextH->launch(0, 1);
}

void initOptixPrime(std::vector<Vertex> &vertices) {
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

void doOptixPrime(double xpos, double ypos) {
	hits.resize(optixW*optixH);
	optix::float3 origin = optix::make_float3(0.0f, 0.0f, -7.0f);
	optix::float3 upperLeftCorner = optix::make_float3(3.0f, 3.0f, -3.0f);
	float step = 6.0f / 512.0f;
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> rays;
	rays.resize(optixW*optixH * 2);
	for (size_t j = 0; j < optixH; j++) {
		for (size_t i = 0; i < optixW; i++) {
			rays[(j*optixH + i) * 2] = origin;
			rays[((j*optixH + i) * 2) + 1] = optix::normalize(upperLeftCorner - optix::make_float3(i*6.0f/optixW, j*6.0f/optixH, 0) - origin);
		}

	}
	query->setRays(optixW*optixH, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T, RTP_BUFFER_TYPE_HOST, hits.data());
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
			hits[(j*optixH + i)] = (hits[(j*optixH + i)]>0) ? 1.0f : 0.0f;
		}

	}
}

void intersectMouse(double xpos, double ypos) {
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_ANY);
	std::vector<optix::float3> ray = { optix::make_float3(0, 0, -7), optix::make_float3(xpos*3.0, ypos*3.0, 1) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	std::vector<float> hit;
	hit.resize(1);
	query->setHits(1, RTP_BUFFER_FORMAT_HIT_T, RTP_BUFFER_TYPE_HOST, hit.data());
	try{
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	query->finish();
	if (hit[0] > 0) printf("hit!");
	else printf("miss!");

}

void debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram) {
	// creating vao
	glGenVertexArrays(1, &linevao);

	// creating vertexbuffer for the vao
	glGenBuffers(1, &linevbo);

	GLuint vertexShader;
	loadShader(vertexShader, "debugshader.vert", GL_VERTEX_SHADER);

	GLuint fragmentShader;
	loadShader(fragmentShader, "shader.frag", GL_FRAGMENT_SHADER);

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
	glDrawArrays(GL_LINES, 0, 2);
}

// OpenGL debug callback
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		std::cerr << "OpenGL: " << message << std::endl;
	}
}

// helper function to check if file exists
inline bool exists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

// making these global is a very elegant solution for the stack overflow 
// that would inevitably follow when declaring it in the function scope where it actually belongs
GLubyte encodepixels[3 * WIDTH * HEIGHT];

/*
Encode from raw pixels to disk with a single function call
The image argument has width * height RGBA pixels or width * height * 4 bytes
*/
void encodeOneStep(const char* filename, const char* extension, unsigned width, unsigned height)
{
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, encodepixels);
	if (GL_NO_ERROR != glGetError()) throw "Error: Unable to read pixels.";

	std::string temp = filename;
	temp.append(extension);

	// decide upon suitabe outputname:
	int i = 0;
	while (exists(temp)) {
		temp = filename;
		temp.append(std::to_string(i));
		temp.append(extension);
		i++;
	}
	// decide upon name
	char * name = new char[temp.length() + 1];
	strcpy(name, temp.c_str());

	/*Encode the image*/
	unsigned error = lodepng_encode24_file(name, encodepixels, width, height);

	/*if there's an error, display it*/
	if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
}

// key button callback to print screen
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (key == GLFW_KEY_P && action == GLFW_PRESS){
		std::cout << "print" << std::endl;
		// write png image
		encodeOneStep("output",".png", WIDTH, HEIGHT);
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
			left = false;
			}
		else {
			debugline.at(1) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
			left = true;
		}
		intersectMouse(xpos, ypos);
		//doOptix(xpos, ypos);
		//doOptixPrime(0, 0);
	}



}

void initRes(GLuint &shaderProgram, GLuint &vao, GLuint &texToon) {
	GLuint vertexShader;
	loadShader(vertexShader, "optixShader.vert", GL_VERTEX_SHADER);

	GLuint fragmentShader;
	loadShader(fragmentShader, "optixShader.frag", GL_FRAGMENT_SHADER);

	// Combine vertex and fragment shaders into single shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	if (!checkProgramErrors(shaderProgram)) {
		std::cerr << "Program failed to link!" << std::endl;
		//return EXIT_FAILURE;
	}

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
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// The position vectors should be retrieved from the specified Vertex Buffer Object with given offset and stride
	// Stride is the distance in bytes between vertices
	//INFO: glVertexAttribPointer always loads the data from GL_ARRAY_BUFFER, and puts it into the VertexArray
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// Create Texture
	// everything will now be done in GL_TEXTURE1
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &texToon);
	glBindTexture(GL_TEXTURE_2D, texToon);

	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, optixW, optixH, 0, GL_RED, GL_FLOAT, hits.data());

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

	std::vector<Vertex> vertices;

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
	initOptixPrime(vertices);

	//initializing result optix drawing
	GLuint optixTex, optixVao, optixShader;
	doOptixPrime(0, 0);
	initRes(optixShader, optixVao, optixTex);
	std::cout << optixTex << std::endl;

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	debuglineInit(linevao, linevbo, debugprogram);


	// Main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Clear the framebuffer to black and depth to maximum value
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//drawRegular(shaderProgram, vao, vertices);
		drawRes(optixShader, optixVao);
		//std::cout << "whatever" << std::endl;

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