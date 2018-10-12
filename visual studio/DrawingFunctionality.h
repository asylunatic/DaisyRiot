#pragma once
#include <GL/glew.h>
#include "ShaderLoader.h"
#include <glm/glm.hpp>

class DrawingFunctionality
{
public:
	DrawingFunctionality();
	~DrawingFunctionality();
	static void debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram);
	static void debuglineDraw(GLuint &debugprogram, GLuint &linevao, GLuint &linevbo);
	static void setResDrawing();
	static void refreshTexture();
	static void initRes(GLuint &shaderProgram);
	static void drawRes(GLuint &shaderProgram, GLuint &vao);

};

