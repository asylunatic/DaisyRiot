#include "DrawingFunctionality.h"
#include <GLFW/glfw3.h>

DrawingFunctionality::DrawingFunctionality()
{
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

void refreshTexture() {
	glActiveTexture(GL_TEXTURE1);
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



DrawingFunctionality::~DrawingFunctionality()
{
}
