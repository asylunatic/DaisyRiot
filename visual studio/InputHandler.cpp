#include "InputHandler.h"
//// key button callback to print screen
//void InputHandler::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
//	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
//		std::cout << "print" << std::endl;
//		// write png image
//		int WIDTH, HEIGHT;
//		glfwGetWindowSize(window, &WIDTH, &HEIGHT);
//		ImageExporter ie = ImageExporter(WIDTH, HEIGHT);
//		glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, ie.encodepixels);
//		if (GL_NO_ERROR != glGetError()) throw "Error: Unable to read pixels.";
//		ie.encodeOneStep("screenshots/output", ".png", WIDTH, HEIGHT);
//	}
//	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
//		std::cout << "left" << std::endl;
//		eye = eye - optix::make_float3(0.5f, 0.0f, 0.0f);
//		viewDirection = viewDirection - optix::make_float3(0.0005f, 0.0f, 0.0f);
//		doOptixPrime();
//		Drawer::refreshTexture(optixW, optixH, optixView);
//	}
//
//	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
//		std::cout << "right" << std::endl;
//		eye = eye + optix::make_float3(0.5f, 0.0f, 0.0f);
//		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
//		doOptixPrime();
//		Drawer::refreshTexture(optixW, optixH, optixView);
//	}
//
//	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
//		std::cout << "up" << std::endl;
//		eye = eye + optix::make_float3(0.0f, 0.5f, 0.0f);
//		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
//
//		doOptixPrime();
//		Drawer::refreshTexture(optixW, optixH, optixView);
//	}
//
//	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
//		std::cout << "down" << std::endl;
//		eye = eye - optix::make_float3(0.0f, 0.5f, 0.0f);
//		viewDirection = viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
//		doOptixPrime();
//		Drawer::refreshTexture(optixW, optixH, optixView);
//	}
//
//	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
//		float formfactor = p2pFormfactor2(patches[0].triangleId, patches[1].triangleId);
//	}
//
//	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
//		int id;
//		std::cout << "\nenter the id of the triangle you want to find" << std::endl;
//		std::cin >> id;
//		for (MatrixIndex index : trianglesonScreen[id]) {
//			optixView[(index.row*optixH + index.col)] = glm::vec3(0.0, 1.0, 0.0);
//		}
//		Drawer::refreshTexture(optixW, optixH, optixView);
//	}
//}
void InputHandler::leftclick(GLFWwindow* window, bool &left, bool &hitB, std::vector<Vertex> &debugline, int optixW, int optixH, optix::float3 &viewDirection, optix::float3 &eye, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
	optix::prime::Model &model, std::vector<glm::vec3> &optixView, std::vector<OptixFunctionality::Hit> &patches, std::vector<Vertex> &vertices) {
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	xpos = xpos * 2 / width - 1;
	ypos = 1 - ypos * 2 / height;
	printf("\nclick!: %f %f", xpos, ypos);
	if (left) {
		debugline.at(0) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
	}
	else {
		debugline.at(1) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
	}
	hitB = OptixPrimeFunctionality::intersectMouse(left, xpos, ypos, optixW, optixH, viewDirection, eye, trianglesonScreen,
		model, optixView, patches, vertices);
}
//void InputHandler::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
//{
//	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
//		InputHandler::leftclick(window, left, hitB, debugline, optixW, optixH, viewDirection, eye, trianglesonScreen,
//			model, optixView, patches, vertices);
//	}
//}


