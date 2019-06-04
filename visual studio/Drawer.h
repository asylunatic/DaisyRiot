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
#include "MeshS.h"

// some forward declarations here bc poor design is my signature
struct Camera;
class OptixPrimeFunctionality;
class Lightning;

class Drawer
{
public:
	struct DebugLine{
		bool cleared;
		bool left;
		bool hitB;
		std::vector<vertex::Vertex> line;
		std::vector<int> debugtriangles;
		DebugLine(){
			cleared = true;
			left = true;
			hitB = false;
			line = { { glm::vec3(0.0f, 0.0f, 0.0f),
						glm::vec3(0.0f, 1.0f, 0.0f) },
						{ glm::vec3(0.0f, 0.0f, 0.0f),
						glm::vec3(0.0f, 1.0f, 0.0f) } };
		};
		void reset(){
			cleared = true;
			left = true;
			hitB = false;
			line.at(0) = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
			line.at(1) = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
			debugtriangles.clear();
		}
	};

	struct RenderContext{
		RenderContext(std::vector<std::vector<MatrixIndex>> &trianglesonScreen, Lightning &lightning, std::vector<glm::vec3> &optixView, MeshS &mesh, Camera &camera, 
			GLuint &debugprogram, GLuint &linevao, GLuint &linevbo, bool &radiosityRendering) :
			trianglesonScreen(trianglesonScreen), lightning(lightning), optixView(optixView), mesh(mesh), camera(camera), debugprogram(debugprogram), linevao(linevao), 
			linevbo(linevbo), radiosityRendering(radiosityRendering){};
		std::vector<std::vector<MatrixIndex>> &trianglesonScreen;
		Lightning &lightning;
		std::vector<glm::vec3> &optixView;
		MeshS &mesh;
		Camera &camera;
		GLuint &debugprogram;
		GLuint &linevao;
		GLuint &linevbo;
		bool &radiosityRendering;
	};

	static GLFWwindow* initWindow(int width, int height);
	static void debuglineInit(GLuint &linevao, GLuint &linevbo, GLuint &shaderProgram);
	static void debuglineDraw(Drawer::RenderContext &rendercontext, Drawer::DebugLine &debugline);
	static void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
	static void setResDrawing(GLuint &optixVao, GLuint &optixTex, int optixW, int optixH, std::vector<glm::vec3> &optixView);
	static void refreshTexture(Drawer::RenderContext &rendercontext);
	static void initRes(GLuint &shaderProgram, GLuint &optixVao, GLuint &optixTex, int width, int height, std::vector<glm::vec3> &optixView);
	static void drawRes(GLuint &shaderProgram, GLuint &vao);
	static glm::vec3 interpolate(MatrixIndex& index, int triangleId, Lightning &lightning, MeshS& mesh);
	static void draw(GLFWwindow* window, GLuint &optixShader, GLuint &optixVao, Drawer::DebugLine &debugline, OptixPrimeFunctionality &optixP, Drawer::RenderContext &rendercontext);
	static void debugtrianglesDraw(DebugLine &debugline, RenderContext &rendercontext);
};

