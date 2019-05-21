#pragma once
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
#include <Eigen/Sparse>

#include "ShaderLoader.h"
#include "Vertex.h"
#include "Defines.h"

// some forward declarations here bc poor design is my signature
struct Camera;
class OptixPrimeFunctionality;

class Drawer
{
public:
	struct DebugLine{
		bool left;
		bool hitB;
		std::vector<vertex::Vertex> line;

		DebugLine(){
			left = true;
			hitB = false;
			line = { { glm::vec3(0.0f, 0.0f, 0.0f),
						glm::vec3(0.0f, 1.0f, 0.0f) },
						{ glm::vec3(0.0f, 0.0f, 0.0f),
						glm::vec3(0.0f, 1.0f, 0.0f) } };
		};
	};

	struct RenderContext{
		RenderContext(std::vector<std::vector<MatrixIndex>> &trianglesonScreen, Eigen::VectorXf &lightningvalues, std::vector<glm::vec3> &optixView, vertex::MeshS &mesh, Camera &camera, 
			GLuint &debugprogram, GLuint &linevao, GLuint &linevbo, bool &radiosityRendering) :
			trianglesonScreen(trianglesonScreen), lightningvalues(lightningvalues), optixView(optixView), mesh(mesh), camera(camera), debugprogram(debugprogram), linevao(linevao), 
			linevbo(linevbo), radiosityRendering(radiosityRendering){};
		std::vector<std::vector<MatrixIndex>> &trianglesonScreen;
		Eigen::VectorXf &lightningvalues;
		std::vector<glm::vec3> &optixView;
		vertex::MeshS &mesh;
		Camera &camera;
		GLuint &debugprogram;
		GLuint &linevao;
		GLuint &linevbo;
		bool &radiosityRendering;
	};

	static GLFWwindow* initWindow(int width, int height);
	static void debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram);
	static void debuglineDraw(GLuint &debugprogram, GLuint &linevao, GLuint &linevbo, DebugLine debugline);
	static void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
	static void setResDrawing(GLuint &optixVao, GLuint &optixTex, int optixW, int optixH, std::vector<glm::vec3> &optixView);
	static void refreshTexture(int optixW, int optixH, std::vector<glm::vec3> &optixView);
	static void initRes(GLuint &shaderProgram, GLuint &optixVao, GLuint &optixTex, int width, int height, std::vector<glm::vec3> &optixView);
	static void drawRes(GLuint &shaderProgram, GLuint &vao);
	static void setRadiosityTex(std::vector<std::vector<MatrixIndex>> &trianglesonScreen, Eigen::VectorXf &lightningvalues, std::vector<glm::vec3> &optixView, int width, int height, vertex::MeshS& mesh);
	static float interpolate(MatrixIndex& index, int triangleId, Eigen::VectorXf &lightningvalues, vertex::MeshS& mesh);
	static void draw(GLFWwindow* window, GLuint &optixShader, GLuint &optixVao, Drawer::DebugLine &debugline, OptixPrimeFunctionality optixP, Drawer::RenderContext rendercontext);
};

