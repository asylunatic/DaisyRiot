#pragma once
#include <sys/stat.h>
#include <sstream>
#include <GL/glew.h>
#include <fstream>
#include <vector>
#include <iostream>

class ShaderLoader
{
public:
	ShaderLoader();
	~ShaderLoader();
	static std::string readFile(const std::string& path);
	static bool checkShaderErrors(GLuint shader);
	static bool checkProgramErrors(GLuint program);
	static void loadShader(GLuint &shader, std::string name, GLenum shaderType);
};

