#include "OptixFunctionality.h"

void OptixFunctionality::initOptix(optix::Context &context, std::vector<Vertex> &vertices) {

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
	rtBufferSetSize1D(buffer, vertices.size());
	void* data;
	rtBufferMap(buffer, &data);
	glm::vec3* vertex_data = (glm::vec3*) data;
	for (int i = 0; i < vertices.size(); i++) {
		vertex_data[i] = vertices[i].normal;
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

void OptixFunctionality::doOptix(optix::Context &context, double &xpos, double &ypos) {

	context["mousePos"]->setFloat((float)xpos, (float)ypos);

	context->launch(0, 1);
}

optix::float3 OptixFunctionality::glm2optixf3(glm::vec3 v) {
	return optix::make_float3(v.x, v.y, v.z);
}

glm::vec3 OptixFunctionality::optix2glmf3(optix::float3 v) {
	return glm::vec3(v.x, v.y, v.z);
}

optix::float3 OptixFunctionality::TriangleMath::uv2xyz(int triangleId, optix::float2 &uv, std::vector<Vertex> &vertices) {
	glm::vec3 a = vertices[triangleId * 3].pos;
	glm::vec3 b = vertices[triangleId * 3 + 1].pos;
	glm::vec3 c = vertices[triangleId * 3 + 2].pos;
	glm::vec3 point = a + uv.x*(b - a) + uv.y*(c - a);
	return optix::make_float3(point.x, point.y, point.z);
}

glm::vec3 OptixFunctionality::TriangleMath::calculateCentre(float triangleId, std::vector<Vertex> &vertices) {
	glm::vec3 centre = (vertices[triangleId * 3].pos + vertices[triangleId * 3 + 1].pos + vertices[triangleId * 3 + 2].pos);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

glm::vec3 OptixFunctionality::TriangleMath::avgNormal(float triangleId, std::vector<Vertex> &vertices) {
	glm::vec3 avg = (vertices[triangleId * 3].normal + vertices[triangleId * 3 + 1].normal + vertices[triangleId * 3 + 2].normal);
	return glm::vec3(avg.x / 3, avg.y / 3, avg.z / 3);
}

float OptixFunctionality::TriangleMath::calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	float theta = glm::acos(glm::dot(ab, ac) / (glm::length(ab)*glm::length(ac)));
	float surface = 0.5 * glm::length(ab)*glm::length(ac) + glm::sin(theta);
	//printf("\nsurface area is: %f", surface);
	return surface;
}

float OptixFunctionality::TriangleMath::calcPointFormfactor(Vertex orig, Vertex dest) {
	float formfactor = 0;
	float dot1 = glm::dot(orig.normal, glm::normalize(dest.pos - orig.pos));
	float dot2 = glm::dot(dest.normal, glm::normalize(orig.pos - dest.pos));
	if (dot1 > 0 && dot2 > 0) {
		float length = glm::sqrt(glm::dot(dest.pos - orig.pos, dest.pos - orig.pos));
		float theta1 = glm::acos(dot1 / length) * 180.0 / M_PIf;
		float theta2 = glm::acos(dot2 / length) * 180.0 / M_PIf;
		printf("\n theta's: %f, %f \nlength: %f \ndots: %f, %f", theta1, theta2, length, dot1, dot2);
		formfactor = dot1 * dot2 / std::powf(length, 4)*M_PIf;
	}

	return formfactor;
}

OptixFunctionality::OptixFunctionality()
{
}


OptixFunctionality::~OptixFunctionality()
{
}
