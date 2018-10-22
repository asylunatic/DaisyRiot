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
#include "ShaderLoader.h"
#include "Vertex.h"

#pragma once
class Drawer
{
public:
	static GLFWwindow* initWindow(int width, int height);
	static void debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram);
	static void debuglineDraw(GLuint &debugprogram, GLuint &linevao, GLuint &linevbo, std::vector<Vertex> &debugline, bool hitB);
	static void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
	static void setResDrawing(GLuint &optixVao, GLuint &optixTex, int optixW, int optixH, std::vector<glm::vec3> &optixView);
	static void refreshTexture(int optixW, int optixH, std::vector<glm::vec3> &optixView);
	static void initRes(GLuint &shaderProgram, GLuint &optixVao, GLuint &optixTex, int optixW, int optixH, std::vector<glm::vec3> &optixView);
	static void drawRes(GLuint &shaderProgram, GLuint &vao);
	static void draw(GLFWwindow* window, GLuint &optixShader, GLuint &optixVao, GLuint &debugprogram, GLuint &linevao, GLuint &linevbo, std::vector<Vertex> &debugline, bool hitB);
};

