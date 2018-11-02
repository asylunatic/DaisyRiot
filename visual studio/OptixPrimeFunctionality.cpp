#include "OptixPrimeFunctionality.h"

void OptixPrimeFunctionality::initOptixPrime(std::vector<Vertex> &vertices) {
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

void  OptixPrimeFunctionality::doOptixPrime(int optixW, int optixH, std::vector<glm::vec3> &optixView,
		optix::float3 &eye, optix::float3 &viewDirection, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, std::vector<Vertex> &vertices) {

	std::vector<Hit> hits;
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

float OptixPrimeFunctionality::p2pFormfactor(int originPatch, int destPatch, std::vector<Vertex> &vertices, std::vector<UV> &rands) {
	glm::vec3 centreOrig = TriangleMath::calculateCentre(originPatch, vertices);
	glm::vec3 centreDest = TriangleMath::calculateCentre(destPatch, vertices);

	float formfactor = OptixFunctionality::TriangleMath::calcPointFormfactor(Vertex(centreOrig, vertices[originPatch * 3].normal), Vertex(centreDest, vertices[destPatch * 3].normal));
	printf("\nformfactor before including surfacearea: %f", formfactor);
	formfactor = formfactor*TriangleMath::calculateSurface(vertices[destPatch * 3].pos, vertices[destPatch * 3 + 1].pos, vertices[destPatch * 3 + 2].pos);

	printf("\nformfactor: %f", formfactor);
	float visibility = OptixPrimeFunctionality::calculateVisibility(originPatch, destPatch, vertices, contextP, model, rands);

	return formfactor * visibility;

}

float OptixPrimeFunctionality::p2pFormfactor2(int originPatch, int destPatch, std::vector<Vertex> &vertices, std::vector<UV> &rands) {
	glm::vec3 centreOrig = TriangleMath::calculateCentre(originPatch, vertices);
	//    A___B<------centreOrig
	//     \ /    ----/
	//      C<---/

	std::vector<glm::vec3> hemitriangle;
	std::vector<glm::vec3> projtriangle;

	projtriangle.resize(3);
	hemitriangle.resize(3);

	for (int i = 0; i < 2; i++) {
		hemitriangle[i] = glm::normalize(vertices[destPatch * 3 + i].pos - centreOrig);
		projtriangle[i] = hemitriangle[i] - glm::dot(vertices[originPatch * 3].normal, hemitriangle[i])*vertices[originPatch * 3].normal;
	}


	float visibility = OptixPrimeFunctionality::calculateVisibility(originPatch, destPatch, vertices, contextP, model, rands);
	float formfactor = TriangleMath::calculateSurface(projtriangle[0], projtriangle[1], projtriangle[2]) / M_PIf;
	printf("\nformfactor: %f \nvisibility: %f", formfactor, visibility);

	return formfactor * visibility;

}

float OptixPrimeFunctionality::calculateVisibility(int originPatch, int destPatch, std::vector<Vertex> &vertices, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands) {
	std::vector<optix::float3> rays;
	rays.resize(2 * RAYS_PER_PATCH);

	std::vector<Hit> hits;
	hits.resize(RAYS_PER_PATCH);

	optix::float3 origin;
	optix::float3 dest;
	for (int i = 0; i < RAYS_PER_PATCH; i++) {
		origin = TriangleMath::uv2xyz(originPatch, optix::make_float2(rands[i].u, rands[i].v), vertices);
		dest = TriangleMath::uv2xyz(destPatch, optix::make_float2(rands[i].u, rands[i].v), vertices);
		rays[i * 2] = origin + optix::normalize(dest - origin)*0.000001f;
		rays[i * 2 + 1] = optix::normalize(dest - origin);
	}

	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	query->setRays(RAYS_PER_PATCH, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
	hitBuffer->setRange(0, RAYS_PER_PATCH);
	query->setHits(hitBuffer);
	try {
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}

	float visibility = 0;


	for (Hit hit : hits) {
		float newT = hit.t > 0 && hit.triangleId == destPatch ? 1 : 0;
		visibility += newT;
	}
	//printf("rays hit: %f", visibility);
	visibility = visibility / RAYS_PER_PATCH;
	return visibility;
}

// a first implementation for the visibility of the point light
float OptixPrimeFunctionality::calculatePointLightVisibility(optix::float3 &lightpos, int patch, std::vector<Vertex> &vertices, std::vector<UV> &rands) {

	std::vector<optix::float3> rays;
	rays.resize(RAYS_PER_PATCH);
	std::vector<Hit> hits;
	hits.resize(RAYS_PER_PATCH);

	optix::float3 origin = lightpos;
	optix::float3 dest;
	for (int i = 0; i < RAYS_PER_PATCH; i++) {
		dest = TriangleMath::uv2xyz(patch, optix::make_float2(rands[i].u, rands[i].v), vertices);
		rays[i] = optix::normalize(dest - origin);
	}

	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	query->setRays(RAYS_PER_PATCH, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
	hitBuffer->setRange(0, RAYS_PER_PATCH);
	query->setHits(hitBuffer);
	try {
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}

	float visibility = 0;

	for (Hit hit : hits) {
		float newT = hit.t > 0 && hit.triangleId == patch ? 1 : 0;
		visibility += newT;
	}
	visibility = visibility / RAYS_PER_PATCH;
	return visibility;
}

// TODO: optimize insertion, this is probably best done with triplets, as explained on:
// https://eigen.tuxfamily.org/dox/group__TutorialSparse.html
void OptixPrimeFunctionality::calculateRadiosityMatrix(SpMat &RadMat, std::vector<Vertex> &vertices, std::vector<UV> &rands) {
	int numtriangles = vertices.size() / 3;
	for (int row = 0; row < numtriangles -1; row++) {
		// calulate form factors current patch to all other patches (that have not been calculated already)
		for (int col = (row +1); col < numtriangles; col++) {
			float formfactorRC = p2pFormfactor2(row, col, vertices, rands);
			if (formfactorRC != 0) {
				//std::cout << "non zero entry should be set: " << formfactorRC << std::endl;
				RadMat.insert(row, col) = formfactorRC;
				// The reciprocity theorem for view factors allows one to calculate F_c->r if one already knows F_r->c.
				// Using the areas of the two surfaces A_a and A_b:
				// A_r*F_r->c = A_c*F_c->r
				// ( A_r*F_r->c)/A_c = F_c->r
				float area_r = TriangleMath::calculateSurface(vertices[row * 3].pos, vertices[row * 3 + 1].pos, vertices[row * 3 + 2].pos);
				float area_c = TriangleMath::calculateSurface(vertices[col * 3].pos, vertices[col * 3 + 1].pos, vertices[col * 3 + 2].pos);
				float formfactorCR = (area_r * formfactorRC) / area_c;
				RadMat.insert(col, row) = formfactorCR;
			}
		}
	}
}

bool OptixPrimeFunctionality::shootPatchRay(std::vector<Hit> &patches, std::vector<Vertex> &vertices) {
	optix::float3  pointA = TriangleMath::uv2xyz(patches[0].triangleId, patches[0].uv, vertices);
	optix::float3  pointB = TriangleMath::uv2xyz(patches[1].triangleId, patches[1].uv, vertices);
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> ray = { pointA + optix::normalize(pointB - pointA)*0.000001f, optix::normalize(pointB - pointA) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	Hit hit;
	query->setHits(1, RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, &hit);
	try {
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	if (hit.triangleId == patches[1].triangleId) {
		return true;
	}
	else return false;
}

bool OptixPrimeFunctionality::intersectMouse(bool &left, double xpos, double ypos, int optixW, int optixH, optix::float3 &viewDirection, optix::float3 &eye, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
	std::vector<glm::vec3> &optixView, std::vector<Hit> &patches, std::vector<Vertex> &vertices) {
	bool hitB = true;
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> ray = { eye, optix::normalize(optix::make_float3(xpos + viewDirection.x, ypos + viewDirection.y, viewDirection.z)) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	OptixFunctionality::Hit hit;
	query->setHits(1, RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, &hit);
	try {
		query->execute(RTP_QUERY_HINT_NONE);
	}
	catch (optix::prime::Exception &e) {
		std::cerr << "An error occurred with error code "
			<< e.getErrorCode() << " and message "
			<< e.getErrorString() << std::endl;
	}
	query->finish();
	if (hit.t > 0) {
		printf("\nhit triangle: %i ", hit.triangleId);
		if (left) patches[0] = hit;
		else {
			patches[1] = hit;
			printf("\nshoot ray between patches \n");
			printf("patch triangle 1: %i \n", patches[0].triangleId);
			printf("patch triangle 2: %i \n", patches[1].triangleId);
			hitB = shootPatchRay(patches, vertices);
			printf("\ndid it hit? %i", hitB);
		}
		left = !left;
		for (MatrixIndex index : trianglesonScreen[hit.triangleId]) {
			optixView[(index.row*optixH + index.col)] = glm::vec3(1.0, 1.0, 1.0);
		}
		Drawer::refreshTexture(optixW, optixH, optixView);
	}
	else {
		printf("miss!");
		hitB = false;
		patches.clear();
		patches.resize(2);
		left = true;
	}
	return hitB;
}