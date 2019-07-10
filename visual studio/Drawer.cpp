#include "Drawer.h"
#include "InputHandler.h"

GLFWwindow* Drawer::initWindow(int width, int height) {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW!" << std::endl;
		//return EXIT_FAILURE;
	}

	// Create window and OpenGL 4.3 debug context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(width, height, "Daisy Riots", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create OpenGL context!" << std::endl;
		//return EXIT_FAILURE;
	}

	// Activate the OpenGL context
	glfwMakeContextCurrent(window);

	return window;

}

void Drawer::debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram) {
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

void Drawer::debuglineDraw(Drawer::RenderContext &cntxt, Drawer::DebugLine &debugline) {
	glUseProgram(cntxt.debugprogram);
	glBindVertexArray(cntxt.linevao);
	// vao will get info from linevbo now
	glBindBuffer(GL_ARRAY_BUFFER, cntxt.linevbo);
	//insert data into the current array_buffer: the vbo
	glBufferData(GL_ARRAY_BUFFER, debugline.line.size() * sizeof(vertex::Vertex), debugline.line.data(), GL_STREAM_DRAW);
	// The position vectors should be retrieved from the specified Vertex Buffer Object with given offset and stride
	// Stride is the distance in bytes between vertices
	//INFO: glVertexAttribPointer always loads the data from GL_ARRAY_BUFFER, and puts it into the current VertexArray
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex::Vertex), reinterpret_cast<void*>(offsetof(vertex::Vertex, pos)));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex::Vertex), reinterpret_cast<void*>(offsetof(vertex::Vertex, normal)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	GLuint hitLoc = glGetUniformLocation(cntxt.debugprogram, "hit");
	glUniform1i(hitLoc, debugline.hitB);
	glDrawArrays(GL_LINES, 0, 2);
}

// OpenGL debug callback
void APIENTRY Drawer::debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		std::cerr << "OpenGL: " << message << std::endl;
	}
}

void Drawer::setResDrawing(GLuint &optixVao, GLuint &optixTex, int optixW, int optixH, std::vector<glm::vec3> &optixView) {
	// Create Vertex Buffer Object
	GLuint vbo;
	glGenBuffers(1, &vbo);
	//bind it as (possible) source for the vao
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//insert data into the current array_buffer: the vbo
	std::vector<glm::vec3> quad = { glm::vec3(-1.0f, -1.0f, 0.0f), 
									glm::vec3(1.0f, -1.0f, 0.0f), 
									glm::vec3(-1.0f, 1.0f, 0.0f), 
									glm::vec3(-1.0f, 1.0f, 0.0f), 
									glm::vec3(1.0f, -1.0f, 0.0f), 
									glm::vec3(1.0f, 1.0f, 0.0f) };
	glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(glm::vec3), quad.data(), GL_STATIC_DRAW);

	// Bind vertex data to shader inputs using their index (location)
	// These bindings are stored in the Vertex Array Object
	glGenVertexArrays(1, &optixVao);
	glBindVertexArray(optixVao);

	// The position vectors should be retrieved from the specified Vertex Buffer Object with given offset and stride
	// Stride is the distance in bytes between vertices
	// INFO: glVertexAttribPointer always loads the data from GL_ARRAY_BUFFER, and puts it into the VertexArray
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

void Drawer::refreshTexture(Drawer::RenderContext &cntxt) {
	glActiveTexture(GL_TEXTURE1);
	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cntxt.camera.pixwidth, cntxt.camera.pixheight, 0, GL_RGB, GL_FLOAT, cntxt.optixView.data());
}

void Drawer::initRes(GLuint &shaderProgram, GLuint &optixVao, GLuint &optixTex, int width, int height, std::vector<glm::vec3> &optixView) {
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

	setResDrawing(optixVao, optixTex, width, height, optixView);

	// Set behaviour for when texture coordinates are outside the [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set interpolation for texture sampling (GL_NEAREST for no interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Drawer::drawRes(GLuint &shaderProgram, GLuint &vao) {
	// Bind the shader
	glUseProgram(shaderProgram);

	// bind texture to variable
	GLuint loc;
	loc = glGetUniformLocation(shaderProgram, "tex");
	glUniform1i(loc, 1);

	// Bind vertex data
	glBindVertexArray(vao);
	// Execute draw command
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

glm::vec3 Drawer::interpolate(MatrixIndex& index, int triangleId, Lightning &lightning, MeshS& mesh) {
	UV uv = index.uv;

	//average lightvalues of the three cornerpoints
	glm::vec3 a = { 0.f, 0.f, 0.f };
	glm::vec3 b = { 0.f, 0.f, 0.f };
	glm::vec3 c = { 0.f, 0.f, 0.f };

	for (int adjTriangle : mesh.trianglesPerVertex[mesh.triangleIndices[triangleId].vertex.x]) {
		a += lightning.get_color_of_patch(adjTriangle);
	}
	for (int adjTriangle : mesh.trianglesPerVertex[mesh.triangleIndices[triangleId].vertex.y]) {
		b += lightning.get_color_of_patch(adjTriangle);
	}
	for (int adjTriangle : mesh.trianglesPerVertex[mesh.triangleIndices[triangleId].vertex.z]) {
		c += lightning.get_color_of_patch(adjTriangle);
	}
	a = a / glm::vec3(mesh.trianglesPerVertex[mesh.triangleIndices[triangleId].vertex.x].size());
	b = b / glm::vec3(mesh.trianglesPerVertex[mesh.triangleIndices[triangleId].vertex.y].size());
	c = c / glm::vec3(mesh.trianglesPerVertex[mesh.triangleIndices[triangleId].vertex.z].size());

	float w = (1 - uv.u - uv.v);
	//glm::vec3 lightningvalue = uv.u * b + uv.v * c + w * a;
	glm::vec3 lightningvalue = uv.u * a + uv.v * b + w * c;
	return lightningvalue;
}

void Drawer::debugtrianglesDraw(DebugLine &debugline, RenderContext &rendercontext){
	for (int i = 0; i < debugline.debugtriangles.size(); i++){
		int tri = debugline.debugtriangles[i];
		for (MatrixIndex index : rendercontext.trianglesonScreen[tri]) {
			rendercontext.optixView[(index.row*rendercontext.camera.pixwidth + index.col)] = glm::vec3(1.0, 1.0, 1.0);
		}
	}
}

void Drawer::draw(GLFWwindow* window, GLuint &optixShader, GLuint &optixVao, Drawer::DebugLine &debugline, OptixPrimeFunctionality &optixP, Drawer::RenderContext &rendercontext){
	// Do Optix stuff
	//rendercontext.antialiasing ? optixP.traceScreenAntialiased(rendercontext) : optixP.traceScreen(rendercontext);
	optixP.traceScreen(rendercontext);

	// debug triangles
	if (!debugline.cleared){
		debugtrianglesDraw(debugline, rendercontext);
	}

	Drawer::refreshTexture(rendercontext);

	// Clear the framebuffer to black and depth to maximum value
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Drawer::drawRes(optixShader, optixVao);

	//debugline drawing
	if (!debugline.cleared){
		Drawer::debuglineDraw(rendercontext, debugline);
	}

	// Present result to the screen
	glfwSwapBuffers(window);
}
