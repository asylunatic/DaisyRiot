#include "InputHandler.h"

// And a function to get the context from a window:
InputHandler::callback_context* InputHandler::get_context(GLFWwindow* w) {
	return static_cast<callback_context*>(glfwGetWindowUserPointer(w));
}

InputHandler::callback_context::callback_context(bool & left, bool & hitB, std::vector<Vertex>& debugline, int optixW, int optixH, optix::float3 & viewDirection, 
	optix::float3 & eye, std::vector<std::vector<MatrixIndex>>& trianglesonScreen, std::vector<glm::vec3>& optixView, std::vector<optix_functionality::Hit>& patches, 
	std::vector<Vertex>& vertices, std::vector<UV> &rands, OptixPrimeFunctionality& optixP, Eigen::VectorXf &lightningvalues, Eigen::SparseMatrix<float> &RadMat, Eigen::VectorXf &emission, int &numpasses, Eigen::VectorXf &residualvector, bool &radiosityRendering) :
	left(left), hitB(hitB), debugline(debugline),optixW(optixW),optixH(optixH),viewDirection(viewDirection),eye(eye),trianglesonScreen(trianglesonScreen), optixView(optixView),
	patches(patches), vertices(vertices), rands(rands), optixP(optixP), lightningvalues(lightningvalues), RadMat(RadMat), emission(emission), numpasses(numpasses), residualvector(residualvector), radiosityRendering(radiosityRendering)
{
	
}

void InputHandler::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		InputHandler::leftclick(window);
	}
}

void InputHandler::save_screenshot(GLFWwindow* window){
	std::cout << "print" << std::endl;
	// write png image
	int WIDTH, HEIGHT;
	glfwGetWindowSize(window, &WIDTH, &HEIGHT);
	ImageExporter ie = ImageExporter(WIDTH, HEIGHT);
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, ie.encodepixels);
	if (GL_NO_ERROR != glGetError()) throw "Error: Unable to read pixels.";
	ie.encodeOneStep("screenshots/output", ".png", WIDTH, HEIGHT);
}

void InputHandler::move_left(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "left" << std::endl;
	cbc_ptr->eye = cbc_ptr->eye - optix::make_float3(0.5f, 0.0f, 0.0f);
	cbc_ptr->viewDirection = cbc_ptr->viewDirection - optix::make_float3(0.0005f, 0.0f, 0.0f);
	cbc_ptr->optixP.doOptixPrime(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView, cbc_ptr->eye, cbc_ptr->viewDirection, cbc_ptr->trianglesonScreen, cbc_ptr->vertices);
	if (cbc_ptr->radiosityRendering){
		Drawer::drawRadiosity(cbc_ptr->trianglesonScreen, cbc_ptr->lightningvalues, cbc_ptr->optixView, cbc_ptr->optixW, cbc_ptr->optixH);
	}
	else{
		Drawer::refreshTexture(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView);
	}
}

void InputHandler::move_right(GLFWwindow *window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "right" << std::endl;
	cbc_ptr->eye = cbc_ptr->eye + optix::make_float3(0.5f, 0.0f, 0.0f);
	cbc_ptr->viewDirection = cbc_ptr->viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
	cbc_ptr->optixP.doOptixPrime(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView, cbc_ptr->eye, cbc_ptr->viewDirection, cbc_ptr->trianglesonScreen, cbc_ptr->vertices);
	if (cbc_ptr->radiosityRendering){
		Drawer::drawRadiosity(cbc_ptr->trianglesonScreen, cbc_ptr->lightningvalues, cbc_ptr->optixView, cbc_ptr->optixW, cbc_ptr->optixH);
	}
	else{
		Drawer::refreshTexture(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView);
	}
}

void InputHandler::move_up(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "up" << std::endl;
	cbc_ptr->eye = cbc_ptr->eye + optix::make_float3(0.0f, 0.5f, 0.0f);
	cbc_ptr->viewDirection = cbc_ptr->viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
	cbc_ptr->optixP.doOptixPrime(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView, cbc_ptr->eye, cbc_ptr->viewDirection, cbc_ptr->trianglesonScreen, cbc_ptr->vertices);
	if (cbc_ptr->radiosityRendering){
		Drawer::drawRadiosity(cbc_ptr->trianglesonScreen, cbc_ptr->lightningvalues, cbc_ptr->optixView, cbc_ptr->optixW, cbc_ptr->optixH);
	}
	else{
		Drawer::refreshTexture(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView);
	}
}

void InputHandler::move_down(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "down" << std::endl;
	cbc_ptr->eye = cbc_ptr->eye - optix::make_float3(0.0f, 0.5f, 0.0f);
	cbc_ptr->viewDirection = cbc_ptr->viewDirection + optix::make_float3(0.0005f, 0.0f, 0.0f);
	cbc_ptr->optixP.doOptixPrime(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView, cbc_ptr->eye, cbc_ptr->viewDirection, cbc_ptr->trianglesonScreen, cbc_ptr->vertices);
	if (cbc_ptr->radiosityRendering){
		Drawer::drawRadiosity(cbc_ptr->trianglesonScreen, cbc_ptr->lightningvalues, cbc_ptr->optixView, cbc_ptr->optixW, cbc_ptr->optixH);
	}
	else{
		Drawer::refreshTexture(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView);
	}
}

void InputHandler::calculate_form_vector(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	float ff = cbc_ptr->optixP.p2pFormfactorNusselt(cbc_ptr->patches[0].triangleId, cbc_ptr->patches[1].triangleId, cbc_ptr->vertices, cbc_ptr->rands);
	std::cout << "Form factor = " << ff << std::endl;

}

void InputHandler::find_triangle_by_id(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	int id;
	std::cout << "\nenter the id of the triangle you want to find" << std::endl;
	std::cin >> id;
	for (MatrixIndex index : cbc_ptr->trianglesonScreen[id]) {
		cbc_ptr->optixView[(index.row*cbc_ptr->optixH + index.col)] = glm::vec3(0.0, 1.0, 0.0);
	}
	Drawer::refreshTexture(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView);
}

// key button callback to print screen
void InputHandler::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (key == GLFW_KEY_P && action == GLFW_PRESS){
		save_screenshot(window);
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		move_left(window);
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		move_right(window);
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		move_up(window);
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		move_down(window);
	}

	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		calculate_form_vector(window);
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		find_triangle_by_id(window);
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		std::cout << "Toggle view" << std::endl;
		toggle_view(window);
	}

	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		std::cout << "Add another light pass and set radiosity drawing as result" << std::endl;
		increment_lightpasses(window);
	}

	if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		std::cout << "Converging lightning calculation" << std::endl;
		increment_lightpasses(window);
	}

	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		std::cout << "Clear radiosity drawing" << std::endl;
		clear_light(window);
	}
}

void InputHandler::toggle_view(GLFWwindow* window) {
	callback_context* cbc_ptr = get_context(window);
	cbc_ptr->radiosityRendering = !cbc_ptr->radiosityRendering;

	cbc_ptr->eye = cbc_ptr->eye - optix::make_float3(0.0f, 0.0f, 0.0f);
	cbc_ptr->viewDirection = cbc_ptr->viewDirection + optix::make_float3(0.0f, 0.0f, 0.0f);
	cbc_ptr->optixP.doOptixPrime(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView, cbc_ptr->eye, cbc_ptr->viewDirection, cbc_ptr->trianglesonScreen, cbc_ptr->vertices);
	if (cbc_ptr->radiosityRendering){
		Drawer::drawRadiosity(cbc_ptr->trianglesonScreen, cbc_ptr->lightningvalues, cbc_ptr->optixView, cbc_ptr->optixW, cbc_ptr->optixH);
	}
	else{
		Drawer::refreshTexture(cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->optixView);
	}
}

void InputHandler::increment_lightpasses(GLFWwindow* window) {
	callback_context* cbc_ptr = get_context(window);
	// calculate consecutive lighting pass
	cbc_ptr->residualvector = cbc_ptr->RadMat * cbc_ptr->residualvector;
	cbc_ptr->lightningvalues = cbc_ptr->lightningvalues + cbc_ptr->residualvector;
	cbc_ptr->numpasses++;

	std::cout << "Number of light passes " << cbc_ptr->numpasses << ". Amount of residual light in scene "<< cbc_ptr->residualvector.sum() << std::endl;
	std::cout << "Residual vector: " << std::endl << cbc_ptr->residualvector << std::endl;
	Drawer::drawRadiosity(cbc_ptr->trianglesonScreen, cbc_ptr->lightningvalues, cbc_ptr->optixView, cbc_ptr->optixW, cbc_ptr->optixH);
}

void InputHandler::clear_light(GLFWwindow* window) {
	callback_context* cbc_ptr = get_context(window);
	// clear light passes
	cbc_ptr->residualvector = cbc_ptr->emission;
	cbc_ptr->lightningvalues = cbc_ptr->emission;
	cbc_ptr->numpasses = 0;

	Drawer::drawRadiosity(cbc_ptr->trianglesonScreen, cbc_ptr->lightningvalues, cbc_ptr->optixView, cbc_ptr->optixW, cbc_ptr->optixH);
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
	cbc_ptr->hitB = cbc_ptr->optixP.intersectMouse(cbc_ptr->left, xpos, ypos, cbc_ptr->optixW, cbc_ptr->optixH, cbc_ptr->viewDirection, cbc_ptr->eye, cbc_ptr->trianglesonScreen,
		cbc_ptr->optixView, cbc_ptr->patches, cbc_ptr->vertices);
}
