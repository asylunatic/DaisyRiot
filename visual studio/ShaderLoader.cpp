#include "ShaderLoader.h"


ShaderLoader::ShaderLoader()
{
}


ShaderLoader::~ShaderLoader()
{
}

// Helper function to read a file like a shader
std::string ShaderLoader::readFile(const std::string& path) {
	std::ifstream file(path, std::ios::binary);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string R = buffer.str();
	return R;
}

bool ShaderLoader::checkShaderErrors(GLuint shader) {
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

bool ShaderLoader::checkProgramErrors(GLuint program) {
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

void ShaderLoader::loadShader(GLuint &shader, std::string name, GLenum shaderType) {
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