#include "optix_functionality.h"

void optix_functionality::initOptix(optix::Context &context, MeshS& mesh) {

	/* *******************************************************OPTIX******************************************************* */
	//initializing context -> holds all programs and variables
	context = optix::Context::create();

	//enable printf shit
	context->setPrintEnabled(true);
	context->setPrintBufferSize(4096);

	//setting entry point: a ray generationprogram
	context->setEntryPointCount(1);
	optix::Program origin = context->createProgramFromPTXFile("ptx\\FirstTry.ptx", "raytraceExecution");
	origin->validate();
	context->setRayGenerationProgram(0, origin);


	//initialising buffers and loading normals into buffer
	RTbuffer buffer;
	rtBufferCreate(context->get(), RT_BUFFER_INPUT, &buffer);
	rtBufferSetFormat(buffer, RT_FORMAT_USER);
	rtBufferSetElementSize(buffer, sizeof(glm::vec3));
	rtBufferSetSize1D(buffer, mesh.vertices.size());
	void* data;
	rtBufferMap(buffer, &data);
	glm::vec3* vertex_data = (glm::vec3*) data;
	for (int i = 0; i < mesh.normals.size(); i++) {
		vertex_data[i] = mesh.normals[i];
	}
	rtBufferUnmap(buffer);

	optix::Geometry geometryH = context->createGeometry();
	optix::Material materialH = context->createMaterial();
	optix::GeometryInstance geometryInstanceH = context->createGeometryInstance();
	optix::GeometryGroup geometryGroupH = context->createGeometryGroup();

	optix::Program intersection;
	try {
		intersection = context->createProgramFromPTXFile("ptx\\geometry.ptx", "intersection");
	}
	catch (optix::Exception e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	intersection->validate();

	optix::Program boundingbox = context->createProgramFromPTXFile("ptx\\geometry.ptx", "boundingbox");
	boundingbox->validate();

	geometryH->setPrimitiveCount(1);
	geometryH->setIntersectionProgram(intersection);
	geometryH->setBoundingBoxProgram(boundingbox);

	geometryInstanceH->setMaterialCount(1);
	geometryInstanceH->setGeometry(geometryH);
	geometryInstanceH->setMaterial(0, materialH);

	geometryGroupH->setChildCount(1);
	geometryGroupH->setChild(0, geometryInstanceH);
	geometryGroupH->setAcceleration(context->createAcceleration("NoAccel"));

	context["top_object"]->set(geometryGroupH);

	printf("check");
	/* *******************************************************OPTIX******************************************************* */
}

void optix_functionality::doOptix(optix::Context &context, double &xpos, double &ypos) {

	context["mousePos"]->setFloat((float)xpos, (float)ypos);

	context->launch(0, 1);
}

optix::float3 optix_functionality::glm2optixf3(glm::vec3 v) {
	return optix::make_float3(v.x, v.y, v.z);
}

glm::vec3 optix_functionality::optix2glmf3(optix::float3 v) {
	return glm::vec3(v.x, v.y, v.z);
}

