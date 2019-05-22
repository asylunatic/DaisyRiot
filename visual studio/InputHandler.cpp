#include "InputHandler.h"

InputHandler::InputHandler(){
	leftbuttonpressed = false;
	old_x = 0.0;
	old_y = 0.0;
}

// And a function to get the context from a window:
callback_context* InputHandler::get_context(GLFWwindow* w) {
	return static_cast<callback_context*>(glfwGetWindowUserPointer(w));
}

callback_context::callback_context(Drawer::DebugLine &debugline, Camera &camera, std::vector<std::vector<MatrixIndex>>& trianglesonScreen, std::vector<glm::vec3>& optixView,
	std::vector<optix_functionality::Hit>& patches, vertex::MeshS& mesh, std::vector<UV> &rands, OptixPrimeFunctionality& optixP, Eigen::VectorXf &lightningvalues, Eigen::SparseMatrix<float> &RadMat,
	Eigen::VectorXf &emission, int &numpasses, Eigen::VectorXf &residualvector, bool &radiosityRendering, InputHandler &inputhandler) :
	debugline(debugline), camera(camera), trianglesonScreen(trianglesonScreen), optixView(optixView), patches(patches), mesh(mesh), rands(rands), optixP(optixP), lightningvalues(lightningvalues),
	RadMat(RadMat), emission(emission), numpasses(numpasses), residualvector(residualvector), radiosityRendering(radiosityRendering)
{
}

void InputHandler::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		leftclick(window);
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
		leftrelease(window);
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		rightclick(window);
	}
}

// key button callback to print screen
void InputHandler::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	callback_context* cbc_ptr = get_context(window);
	cbc_ptr->debugline.reset();

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS){
		save_screenshot(window);
	}

	if (key == GLFW_KEY_M && action == GLFW_PRESS){
		print_menu();
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
		toggle_view(window);
	}

	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		increment_lightpasses(window);
	}

	if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		calc_full_lightning(window);
	}

	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		clear_light(window);
	}

	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
		zoom_in(window);
	}

	if (key == GLFW_KEY_MINUS  && action == GLFW_PRESS) {
		zoom_out(window);
	}
}

void InputHandler::print_menu(){
	std::ifstream f_menu("print_menu.txt");
	if (f_menu.is_open())
		std::cout << f_menu.rdbuf();
	f_menu.close();
}

void InputHandler::cursor_pos_callback(GLFWwindow * window, double xpos, double ypos)
{
	callback_context* cbc_ptr = get_context(window);
	if (!leftbuttonpressed)
	{
		return;
	}
	// reset debugline
	cbc_ptr->debugline.reset();

	// update yaw and pitch of camera
	double deltax = xpos - old_x;
	double deltay = ypos - old_y;

	double yaw = (180.0 / float(cbc_ptr->camera.pixwidth)) * deltax;
	double pitch = (180.0 / float(cbc_ptr->camera.pixheight)) * deltay;

	cbc_ptr->camera.rotate(yaw, pitch, 0.0);

	old_x = xpos;
	old_y = ypos;
}

void InputHandler::save_screenshot(GLFWwindow* window){
	std::cout << "print" << std::endl;
	// write png image
	int WIDTH, HEIGHT;
	glfwGetWindowSize(window, &WIDTH, &HEIGHT);
	ImageExporter ie = ImageExporter(WIDTH, HEIGHT);
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, ie.encodepixels);
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, ie.encodepixels_flipped);
	for (int x = 0; x < WIDTH; x++){
		for (int y = 0; y < HEIGHT; y++){
			int indexflipped = (y * WIDTH + x) * 3;
			// flip over y axis
			int y_flip = HEIGHT - 1 - y;
			int newindex = (y_flip * WIDTH + x) * 3;
			ie.encodepixels[newindex] = ie.encodepixels_flipped[indexflipped];
			ie.encodepixels[newindex + 1] = ie.encodepixels_flipped[indexflipped + 1];
			ie.encodepixels[newindex + 2] = ie.encodepixels_flipped[indexflipped + 2];
		}
	}
	if (GL_NO_ERROR != glGetError()) throw "Error: Unable to read pixels.";
	ie.encodeOneStep("screenshots/output", ".png", WIDTH, HEIGHT);
}

void InputHandler::move_left(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "left" << std::endl;
	cbc_ptr->camera.rotate(-10.0, 0.0, 0.0);
}

void InputHandler::move_right(GLFWwindow *window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "right" << std::endl;
	cbc_ptr->camera.rotate(10.0, 0.0, 0.0);
}

void InputHandler::move_up(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "up" << std::endl;
	cbc_ptr->camera.rotate(0.0, 10.0, 0.0);
}

void InputHandler::move_down(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	std::cout << "down" << std::endl;
	cbc_ptr->camera.rotate(0.0, -10.0, 0.0);
}

void InputHandler::calculate_form_vector(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	float ff = cbc_ptr->optixP.p2pFormfactorNusselt(cbc_ptr->patches[0].triangleId, cbc_ptr->patches[1].triangleId, cbc_ptr->mesh, cbc_ptr->rands);
	std::cout << "Form factor = " << ff << std::endl;
}

void InputHandler::find_triangle_by_id(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	int id;
	std::cout << "\nenter the id of the triangle you want to find" << std::endl;
	std::cin >> id;
	cbc_ptr->debugline.debugtriangles.push_back(id);
	cbc_ptr->debugline.cleared = false;
}

void InputHandler::zoom_out(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	cbc_ptr->camera.eye = cbc_ptr->camera.eye*2.0;
}

void InputHandler::zoom_in(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	cbc_ptr->camera.eye = cbc_ptr->camera.eye*0.5;
}

void InputHandler::calc_full_lightning(GLFWwindow* window){
	std::cout << "Converging lightning calculation" << std::endl;
	callback_context* cbc_ptr = get_context(window);
	while (cbc_ptr->residualvector.sum() > 0.0001){
		cbc_ptr->residualvector = cbc_ptr->RadMat * cbc_ptr->residualvector;
		cbc_ptr->lightningvalues = cbc_ptr->lightningvalues + cbc_ptr->residualvector;
		cbc_ptr->numpasses++;
	}
	std::cout << "Number of light passes " << cbc_ptr->numpasses << ". Amount of residual light in scene " << cbc_ptr->residualvector.sum() << std::endl;
}

void InputHandler::toggle_view(GLFWwindow* window) {
	std::cout << "Toggle view" << std::endl;
	callback_context* cbc_ptr = get_context(window);
	cbc_ptr->radiosityRendering = !cbc_ptr->radiosityRendering;
}

void InputHandler::increment_lightpasses(GLFWwindow* window) {
	std::cout << "Add another light pass" << std::endl;
	callback_context* cbc_ptr = get_context(window);
	// calculate consecutive lighting pass
	cbc_ptr->residualvector = cbc_ptr->RadMat * cbc_ptr->residualvector;
	cbc_ptr->lightningvalues = cbc_ptr->lightningvalues + cbc_ptr->residualvector;
	cbc_ptr->numpasses++;
	std::cout << "Number of light passes " << cbc_ptr->numpasses << ". Amount of residual light in scene " << cbc_ptr->residualvector.sum() << std::endl;
}

void InputHandler::clear_light(GLFWwindow* window) {
	std::cout << "Reset light passes to 0" << std::endl;
	callback_context* cbc_ptr = get_context(window);
	// clear light passes
	cbc_ptr->residualvector = cbc_ptr->emission;
	cbc_ptr->lightningvalues = cbc_ptr->emission;
	cbc_ptr->numpasses = 0;
}

void InputHandler::leftclick(GLFWwindow* window)
{
	callback_context* cbc_ptr = get_context(window);
	leftbuttonpressed = true;
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	old_x = xpos;
	old_y = ypos;
}

void InputHandler::leftrelease(GLFWwindow* window){
	callback_context* cbc_ptr = get_context(window);
	leftbuttonpressed = false;
}

void InputHandler::rightclick(GLFWwindow* window)
{
	callback_context* cbc_ptr = get_context(window);
	cbc_ptr->debugline.cleared = false;
	int width, height;
	double xpos, ypos;
	glfwGetWindowSize(window, &width, &height);
	glfwGetCursorPos(window, &xpos, &ypos);
	printf("\nclick!: %f %f", xpos, ypos);
	// calculate intersection
	double ypos_corrected = height - ypos;
	cbc_ptr->debugline.hitB = cbc_ptr->optixP.intersectMouse(cbc_ptr->debugline, xpos, ypos_corrected, cbc_ptr->camera, cbc_ptr->trianglesonScreen,
		cbc_ptr->optixView, cbc_ptr->patches, cbc_ptr->mesh);

	// adjust debug line
	xpos = xpos * 2 / width - 1;
	ypos = 1 - ypos * 2 / height;
	if (cbc_ptr->debugline.left) {
		cbc_ptr->debugline.line.at(0) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
	}
	else {
		cbc_ptr->debugline.line.at(1) = { glm::vec3((float)xpos, (float)ypos, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
	}
}
