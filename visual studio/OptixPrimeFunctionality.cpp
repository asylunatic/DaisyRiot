#include "OptixPrimeFunctionality.h"

void OptixPrimeFunctionality::initOptixPrime(optix::prime::Context &contextP, optix::prime::Model &model, std::vector<Vertex> &vertices) {
	contextP = optix::prime::Context::create(RTP_CONTEXT_TYPE_CUDA);
	optix::prime::BufferDesc buffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_VERTEX_FLOAT3, RTP_BUFFER_TYPE_HOST, vertices.data());
	buffer->setRange(0, vertices.size());
	buffer->setStride(sizeof(Vertex));
	model = contextP->createModel();
	model->setTriangles(buffer);
	try {
		model->update(RTP_MODEL_HINT_NONE);
		model->finish();
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
}

void  OptixPrimeFunctionality::doOptixPrime(int optixW, int optixH, optix::prime::Context &contextP, std::vector<glm::vec3> &optixView,
		optix::float3 &eye, optix::float3 &viewDirection, optix::prime::Model &model, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, std::vector<Vertex> &vertices) {

	std::vector<OptixFunctionality::Hit> hits;
	hits.resize(optixW*optixH);
	optixView.resize(optixW*optixH);
	optix::float3 upperLeftCorner = eye + viewDirection + optix::make_float3(-1.0f, 1.0f, 0.0f);
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> rays;
	rays.resize(optixW*optixH * 2);

	//timing
	std::clock_t start;
	double duration;
	start = std::clock();
	for (size_t j = 0; j < optixH; j++) {
		for (size_t i = 0; i < optixW; i++) {
			rays[(j*optixH + i) * 2] = eye;
			rays[((j*optixH + i) * 2) + 1] = optix::normalize(upperLeftCorner + optix::make_float3(i*2.0f / optixW, -1.0f*(j*2.0f / optixH), 0) - eye);
		}

	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "for looping: " << duration << '\n';
	start = std::clock();

	query->setRays(optixW*optixH, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
	hitBuffer->setRange(0, optixW*optixH);
	query->setHits(hitBuffer);

	try {
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	query->finish();

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "optix rayshooting " << duration << '\n';
	start = std::clock();

	trianglesonScreen.clear();
	trianglesonScreen.resize(vertices.size() / 3);

	for (int j = 0; j < optixH; j++) {
		for (int i = 0; i < optixW; i++) {
			optixView[(j*optixH + i)] = (hits[(j*optixH + i)].t > 0) ? glm::vec3(glm::abs(vertices[hits[(j*optixH + i)].triangleId * 3].normal)) : glm::vec3(0.0f, 0.0f, 0.0f);
			if (hits[(j*optixH + i)].t > 0) {
				MatrixIndex index = {};
				index.col = i;
				index.row = j;
				trianglesonScreen[hits[(j*optixH + i)].triangleId].push_back(index);
			}
		}

	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "for looping again: " << duration << '\n';
	start = std::clock();
}

OptixPrimeFunctionality::OptixPrimeFunctionality()
{
}


OptixPrimeFunctionality::~OptixPrimeFunctionality()
{
}
