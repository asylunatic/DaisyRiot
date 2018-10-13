#include "InputHandler.h"

// And a function to get the context from a window:
InputHandler::callback_context* InputHandler::get_context(GLFWwindow* w) {
	return static_cast<callback_context*>(glfwGetWindowUserPointer(w));
}

InputHandler::callback_context::callback_context(bool & left, bool & hitB, std::vector<Vertex>& debugline, int optixW, int optixH, optix::float3 & viewDirection, 
	optix::float3 & eye, std::vector<std::vector<MatrixIndex>>& trianglesonScreen, optix::prime::Model & model, std::vector<glm::vec3>& optixView, std::vector<OptixFunctionality::Hit>& patches, 
	std::vector<Vertex>& vertices):
	left(left), hitB(hitB), debugline(debugline),optixW(optixW),optixH(optixH),viewDirection(viewDirection),eye(eye),trianglesonScreen(trianglesonScreen),model(model),optixView(optixView),
	patches(patches),vertices(vertices)
{
	
}

void InputHandler::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		InputHandler::leftclick(window);
	}
}

void InputHandler::leftclick(GLFWwindow* window)
{
	callback_context* cbc_ptr = get_context(window);
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	xpos = xpos * 2 / width - 1;
	ypos = 1 - ypos * 2 / height;
	printf("\nclick!: %f %f", xpos, ypos);
	if (cbc_ptr->left) {
		cbc_ptr->debugline.at(0) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
	}
	else {
		cbc_ptr->debugline.at(1) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
	}
	cbc_ptr->hitB = OptixPrimeFunctionality::intersectMouse(cbc_ptr->left, xpos, ypos, cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->viewDirection, cbc_ptr->eye, cbc_ptr->trianglesonScreen,
		cbc_ptr->model, cbc_ptr->optixView, cbc_ptr->patches, cbc_ptr->vertices);
}
