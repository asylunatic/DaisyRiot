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

#include <OptiX_world.h>


// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;


// Per-vertex data
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
};
std::vector<Vertex> debugline = { { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }, 
									{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }};

RTcontext context;
optix::Context contextH;

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

	optix::Geometry geometryH = contextH -> createGeometry();
	optix::Material materialH = contextH->createMaterial();
	optix::GeometryInstance geometryInstanceH = contextH->createGeometryInstance();
	optix::GeometryGroup geometryGroupH = contextH->createGeometryGroup();
	
	optix::Program intersection = contextH->createProgramFromPTXFile("ptx\\geometry.ptx", "intersection");
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

	rtContextLaunch1D(context, 0, 1);
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
	} else {
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
	} else {
		return true;
	}
}

// OpenGL debug callback
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		std::cerr << "OpenGL: " << message << std::endl;
	}
}

void debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram) {
	// creating vao
	glGenVertexArrays(1, &linevao);

	// creating vertexbuffer for the vao
	glGenBuffers(1, &linevbo);

	// Load and compile vertex shader
	std::string vertexShaderCode = readFile("debugshader.vert");
	const char* vertexShaderCodePtr = vertexShaderCode.data();

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderCodePtr, nullptr);
	glCompileShader(vertexShader);

	// Load and compile fragment shader
	std::string fragmentShaderCode = readFile("shader.frag");
	const char* fragmentShaderCodePtr = fragmentShaderCode.data();

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, nullptr);
	glCompileShader(fragmentShader);

	if (!checkShaderErrors(vertexShader) || !checkShaderErrors(fragmentShader)) {
		std::cerr << "Shader(s) failed to compile!" << std::endl;
		return;
	}

	// Combine vertex and fragment shaders into single shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);


}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		xpos = xpos*2/WIDTH - 1;
		ypos = 1 - ypos*2/HEIGHT;
		printf("click!: %f %f", xpos, ypos);
		/*if (debugline.at(0).pos.x == 0.0){
			debugline.at(0) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
			debugline.at(1) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
		}
		else debugline.at(1) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };*/
		doOptix(xpos, ypos);

	}



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

	// Load and compile vertex shader
	std::string vertexShaderCode = readFile("shader.vert");
	const char* vertexShaderCodePtr = vertexShaderCode.data();

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderCodePtr, nullptr);
	glCompileShader(vertexShader);

	// Load and compile fragment shader
	std::string fragmentShaderCode = readFile("shader.frag");
	const char* fragmentShaderCodePtr = fragmentShaderCode.data();

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, nullptr);
	glCompileShader(fragmentShader);

	if (!checkShaderErrors(vertexShader) || !checkShaderErrors(fragmentShader)) {
		std::cerr << "Shader(s) failed to compile!" << std::endl;
		return EXIT_FAILURE;
	}

	// Combine vertex and fragment shaders into single shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	if (!checkProgramErrors(shaderProgram)) {
		std::cerr << "Program failed to link!" << std::endl;
		return EXIT_FAILURE;
	}

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

	// Create Vertex Buffer Object
	GLuint vbo;
	glGenBuffers(1, &vbo);
	//bind it as (possible) source for the vao
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//insert data into the current array_buffer: the vbo
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	// Bind vertex data to shader inputs using their index (location)
	// These bindings are stored in the Vertex Array Object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// The position vectors should be retrieved from the specified Vertex Buffer Object with given offset and stride
	// Stride is the distance in bytes between vertices
	//INFO: glVertexAttribPointer always loads the data from GL_ARRAY_BUFFER, and puts it into the VertexArray
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glEnableVertexAttribArray(0);

	// The normals should be retrieved from the same Vertex Buffer Object (glBindBuffer is optional)
	// The offset is different and the data should go to input 1 instead of 0
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
	glEnableVertexAttribArray(1);

	// Load image
	int width, height, channels;
	stbi_uc* pixels = stbi_load("toon_map.png", &width, &height, &channels, 3);

	// Create Texture
	// everything will now be done in GL_TEXTURE0
	glActiveTexture(GL_TEXTURE0);
	GLuint texToon;
	glGenTextures(1, &texToon);
	glBindTexture(GL_TEXTURE_2D, texToon);

	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// Set behaviour for when texture coordinates are outside the [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set interpolation for texture sampling (GL_NEAREST for no interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	//initializing debugline
	GLuint linevao, linevbo, debugprogram;
	debuglineInit(linevao, linevbo, debugprogram);

	//optix stuff
	initOptix(vertices);
	

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Clear the framebuffer to black and depth to maximum value
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind the shader
		glUseProgram(shaderProgram);

		// Set model/view/projection matrix
		glm::vec3 viewPos = glm::vec3(-0.8f, 0.7f, -10.0f);
		glm::vec3 lightPos = glm::vec3(0.0f, 10.0f, 0.0f);

		glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(45.0f, WIDTH / static_cast<float>(HEIGHT), 0.1f, 10.0f);
		glm::mat4 model = glm::mat4();
		glm::mat4 mvp = proj * view * model;

		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));

		// Set view position
		GLint loc = glGetUniformLocation(shaderProgram, "viewPos");
		glUniform3fv(loc, 1, glm::value_ptr(viewPos));

		// Expose current time in shader uniform
		loc = glGetUniformLocation(shaderProgram, "time");
		glUniform1f(loc, static_cast<float>(glfwGetTime()));

		// setLightPosition
		loc = glGetUniformLocation(shaderProgram, "lightPos");
		glUniform3fv(loc, 1, glm::value_ptr(lightPos));

		// upload texture coordinates
		loc = glGetUniformLocation(shaderProgram, "texToon");
		glUniform1i(loc, 0);

		// Bind vertex data
		glBindVertexArray(vao);
		// Execute draw command
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		//DRAWING DEBUGLINE
		debuglineDraw(debugprogram, linevao, linevbo);

		// Present result to the screen
		glfwSwapBuffers(window);
	}

	// Cleanup
	stbi_image_free(pixels);

    glfwDestroyWindow(window);
	
	glfwTerminate();

    return 0;
}