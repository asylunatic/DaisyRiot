﻿#include "OptixPrimeFunctionality.h"

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
		Camera &camera, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, std::vector<Vertex> &vertices) {

	std::vector<optix_functionality::Hit> hits;
	hits.resize(optixW*optixH);
	optixView.resize(optixW*optixH);
	optix::float3 upperLeftCorner = camera.eye + camera.dir + optix::make_float3(-1.0f, 1.0f, 0.0f);
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> rays;
	rays.resize(optixW*optixH * 2);

	//// generate rays the marijn way
	//for (size_t j = 0; j < optixH; j++) {
	//	for (size_t i = 0; i < optixW; i++) {
	//		rays[(j*optixH + i) * 2] = camera.eye;
	//		rays[((j*optixH + i) * 2) + 1] = optix::normalize(upperLeftCorner + optix::make_float3(i*2.0f / optixW, -1.0f*(j*2.0f / optixH), 0) - camera.eye);
	//	}
	//}

	// generate rays the un_project way
	glm::mat4x4 lookat = glm::lookAt(optix_functionality::optix2glmf3(camera.eye), optix_functionality::optix2glmf3(camera.origin), optix_functionality::optix2glmf3(camera.up));
	glm::mat4x4 projection = glm::ortho(-10.0, 10.0, -10.0, 10.0);
	//glm::mat4x4 projection = glm::perspective(45.0f, (float)(800) / (float)(600), 0.1f, 1000.0f);
	for (size_t x = 0; x < optixW; x++) {
		for (size_t y = 0; y < optixH; y++) {
			glm::vec3 win(x, y, 0.0);
			glm::vec3 world_coord = glm::unProject(win, lookat, projection, camera.viewport);
			rays[(y*optixW + x) * 2] = optix_functionality::glm2optixf3(world_coord);
			optix::float3 dir = (camera.origin - camera.eye);
			dir = normalize(dir);
			rays[((y*optixW + x) * 2) + 1] = dir;
		}
	}

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

	trianglesonScreen.clear();
	trianglesonScreen.resize(vertices.size() / 3);

	for (size_t x = 0; x < optixW; x++) {
		for (size_t y = 0; y < optixH; y++) {
			int pixelIndex = y*optixW + x;
			optixView[pixelIndex] = (hits[pixelIndex].t > 0) ? glm::vec3(glm::abs(vertices[hits[pixelIndex].triangleId * 3].normal)) : glm::vec3(0.0f, 0.0f, 0.0f);
			if (hits[pixelIndex].t > 0 
				&& !triangle_math::isFacingBack(optix_functionality::optix2glmf3(camera.eye), hits[pixelIndex].triangleId, vertices)
				) {
				MatrixIndex index = {};
				index.col = x;
				index.row = y;
				trianglesonScreen[hits[pixelIndex].triangleId].push_back(index);
			}
		}
	}

}

float OptixPrimeFunctionality::p2pFormfactor(int originPatch, int destPatch, std::vector<Vertex> &vertices, std::vector<UV> &rands) {	
	// subdivide triangles and return in vector w/ 12 entries (4*3 coordinates)
	std::vector<std::vector<glm::vec3>> origintriangles, destinationtriangles;
	origintriangles = triangle_math::divideInFourTriangles(originPatch, vertices);
	destinationtriangles = triangle_math::divideInFourTriangles(destPatch, vertices);

	// init vectors
	std::vector<glm::vec3> originpoints, destinationpoints;
	originpoints.resize(4);
	destinationpoints.resize(4);
	
	glm::vec3 originNormal = triangle_math::avgNormal(originPatch, vertices);
	glm::vec3 destNormal = triangle_math::avgNormal(destPatch, vertices);

	// calculate centers of subdivided triangles
	for (int i = 0; i < 4; i++) {
		originpoints[i] = triangle_math::calculateCentre(origintriangles[i]);
		destinationpoints[i] = triangle_math::calculateCentre(destinationtriangles[i]);
	}

	float formfactor = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			formfactor = formfactor +triangle_math::calcPointFormfactor(Vertex(originpoints[i], originNormal), 
				Vertex(destinationpoints[j], destNormal), 
				triangle_math::calculateSurface(origintriangles[i])*triangle_math::calculateSurface(destinationtriangles[i]));
		}
	}
	//printf("\nformfactor before including surfacearea: %f", formfactor);
	formfactor = formfactor / triangle_math::calculateSurface(vertices[originPatch * 3].pos, vertices[originPatch * 3 + 1].pos, vertices[originPatch * 3 + 2].pos);

	//printf("\nformfactor: %f", formfactor);
	float visibility = OptixPrimeFunctionality::calculateVisibility(originPatch, destPatch, vertices, contextP, model, rands);

	return formfactor * visibility;

}

float OptixPrimeFunctionality::calculateVisibility(int originPatch, int destPatch, std::vector<Vertex> &vertices, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands) {
	std::vector<optix::float3> rays;
	rays.resize(2 * RAYS_PER_PATCH);

	std::vector<optix_functionality::Hit> hits;
	hits.resize(RAYS_PER_PATCH);

	optix::float3 origin;
	optix::float3 dest;
	for (int i = 0; i < RAYS_PER_PATCH; i++) {
		origin = triangle_math::uv2xyz(originPatch, optix::make_float2(rands[i].u, rands[i].v), vertices);
		dest = triangle_math::uv2xyz(destPatch, optix::make_float2(rands[i].u, rands[i].v), vertices);
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


	for (optix_functionality::Hit hit : hits) {
		float newT = hit.t > 0 && hit.triangleId == destPatch ? 1 : 0;
		visibility += newT;
	}
	visibility = visibility / RAYS_PER_PATCH;
	return visibility;
}

float OptixPrimeFunctionality::p2pFormfactorNusselt(int originPatch, int destPatch, std::vector<Vertex> &vertices, std::vector<UV> &rands) {

	glm::vec3 center_origin = triangle_math::calculateCentre(originPatch, vertices);
	glm::vec3 center_dest = triangle_math::calculateCentre(destPatch, vertices);

	glm::vec3 normal_origin = triangle_math::avgNormal(originPatch, vertices);

	if (triangle_math::isFacingBack(center_origin, destPatch, vertices),
		triangle_math::isFacingBack(center_dest, originPatch, vertices)) {
		return 0.0;
	}

	// init vectors to project triangle
	std::vector<glm::vec3> hemitriangle;
	std::vector<glm::vec3> projtriangle;
	projtriangle.resize(3);
	hemitriangle.resize(3);

	for (int i = 0; i < 3; i++) {
		hemitriangle[i] = center_origin + glm::normalize(vertices[destPatch * 3 + i].pos - center_origin);

		// calculate the projection of the vertex of the hemi triangle onto the plane that contains the triangle: 
		// (as per https://stackoverflow.com/questions/9605556/how-to-project-a-point-onto-a-plane-in-3d)
		// Given a point-normal definition of a plane with normal n and point o on the plane, a point p', 
		// being the point on the plane closest to the given point p, can be found by:
		// p' = p - (n ⋅ (p - o)) * n
		projtriangle[i] = hemitriangle[i] - (glm::dot(normal_origin, (hemitriangle[i] - center_origin))) * normal_origin;
	}
	
	float visibility = calculateVisibility(originPatch, destPatch, vertices, contextP, model, rands);

	float formfactor = triangle_math::calculateSurface(projtriangle[0], projtriangle[1], projtriangle[2]) / M_PIf;
	float totalformfactor = formfactor * visibility;

	return totalformfactor;

}

// TODO: optimize insertion, this is probably best done with triplets, as explained on:
// https://eigen.tuxfamily.org/dox/group__TutorialSparse.html
void OptixPrimeFunctionality::calculateRadiosityMatrix(SpMat &RadMat, std::vector<Vertex> &vertices, std::vector<UV> &rands) {
	std::cout << "Calculating radiosity matrix..." << std::endl;
	int numtriangles = vertices.size() / 3;
	std::vector<Tripl> tripletList;

	int numfilled = 0;
	for (int row = 0; row < numtriangles -1; row++) {
		// calulate form factors current patch to all other patches (that have not been calculated already):
		// matrix shape should be as follows:
		// *------*------*------*
		// |   0  | 0->1 | 0->2 |
		// *------*------*------*
		// | 1->0 |  0   | 1->2 |
		// *------*------*------*
		// | 2->0 | 2->1 |   0  |
		// *------*------*------*
		//
		// such that we can do the following calculation:
		// M*V1 = V2 where
		// M is radiosity matrix
		// V1 is a vector containing the light that is emitted per patch 
		// V2 is a vector containing the light that is emitted per patch after the bounce

		for (int col = (row +1); col < numtriangles; col++) {
			float formfactorRC = p2pFormfactor(row, col, vertices, rands);
			if (formfactorRC > 0.0) {
				// at place (x, y) we want the form factor y->x 
				// but as this is a col major matrix we store (x, y) at (y, x) -> confused yet?
				tripletList.push_back(Tripl(row, col, formfactorRC));
				//std::cout << "Inserting form factor " << row << "->" << col << " with " << formfactorRC << " at ( " << row << ", " << col << " )" << std::endl;

				// use reprocipity theorem to calculate form factor the other way around
				float formfactorCR = (triangle_math::calculateSurface(row, vertices)*formfactorRC) / triangle_math::calculateSurface(col, vertices);
				tripletList.push_back(Tripl(col, row, formfactorCR));
				//std::cout << "Inserting form factor " << col << "->" << row << " with " << formfactorCR << " at ( " << col << ", " << row << " )" << std::endl;
			}
			numfilled += 2;
		}

		// draw progress bar
		int barWidth = 70;
		float progress = float(float(numfilled) / float(numtriangles*(numtriangles-1)));
		std::cout << "[";
		int pos = barWidth * progress;
		for (int i = 0; i < barWidth; ++i) {
			if (i < pos) std::cout << "=";
			else if (i == pos) std::cout << ">";
			else std::cout << " ";
		}
		std::cout << "] " << int(progress * 100.0) << " %\r";
		std::cout.flush();
	}

	RadMat.setFromTriplets(tripletList.begin(), tripletList.end());
	std::cout << "... done!                                                                                       " << std::endl;
}

void OptixPrimeFunctionality::calculateRadiosityMatrixStochastic(SpMat &RadMat, std::vector<Vertex> &vertices, std::vector<UV> &rands) {
	
	// init some shit
	int numtriangles = vertices.size() / 3;
	int numrays = 1000;
	std::srand(std::time(nullptr)); // use current time as seed for random generator
	std::vector<Tripl> tripletList;
	
	int numfilled = 0;
	for (int row = 0; row < numtriangles; row++) {
		// retrieve origin and normal of triangle w/ row as index
		optix::float3 origin = optix_functionality::glm2optixf3(triangle_math::calculateCentre(row, vertices));
		glm::vec3 rownormal = triangle_math::avgNormal(row, vertices);

		std::vector<optix::float3> rays;
		rays.resize(2 * numrays);
		std::vector<optix_functionality::Hit> hits;
		hits.resize(numrays);

		// generate random rays
		for (int i = 0; i < numrays; i++) {
			
			float x = ((float)(rand() % RAND_MAX)) / RAND_MAX;
			float y = ((float)(rand() % RAND_MAX)) / RAND_MAX;
			float theta = acosf(sqrtf(1.0 - x));
			float phi = 2.0*M_PIf*y;
			
			// rotate normal down by theta, as per https://stackoverflow.com/a/22101541/7925249
			// for d we take a vector perpendicular to the normal, which is luckily just any vector in the plane of the triangle
			glm::vec3 d = glm::normalize(vertices[row*3].pos - vertices[row*3 + 1].pos);
			glm::vec3 temp_down = cos(theta)*rownormal + sin(theta)*d;
			temp_down = glm::normalize(temp_down);
			
			// rotate normal around by phi
			glm::mat4x4 rotation = glm::rotate(glm::mat4(1.0f), phi, rownormal);
			glm::vec3 temp_around = rotation * glm::vec4(temp_down.x, temp_down.y, temp_down.z, 1.0);
			optix::float3 dest = optix_functionality::glm2optixf3(temp_around);
			
			rays[i * 2] = origin + optix::normalize(dest)*0.000001f;
			rays[i * 2 + 1] = optix::normalize(dest);
		}

		// now shoot the rays
		optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
		query->setRays(numrays, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
		optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
		hitBuffer->setRange(0, numrays);
		query->setHits(hitBuffer);
		try {
			query->execute(RTP_QUERY_HINT_NONE);
		}
		catch (optix::prime::Exception &e) {
			std::cerr << "An error occurred with error code "
				<< e.getErrorCode() << " and message "
				<< e.getErrorString() << std::endl;
		}

		// initialize vector with all zeroes and store in this the number of hits
		std::vector<float> hitarray(numtriangles, 0.0);
		for (int i = 0; i < numrays; i++) {
			optix_functionality::Hit hit = hits[i];
			if (hit.t > 0.0){
				// check if we did not hit triangle on the back
				glm::vec3 destcenter = triangle_math::calculateCentre(hit.triangleId, vertices);
				glm::vec3 destnormal = triangle_math::avgNormal(hit.triangleId, vertices);
				float dot1 = glm::dot(rownormal, glm::normalize(destcenter - optix_functionality::optix2glmf3(origin)));
				float dot2 = glm::dot(destnormal, glm::normalize(optix_functionality::optix2glmf3(origin) - destcenter));
				if (dot1 > 0.0 && dot2 > 0.0){
					hitarray[hit.triangleId] += 1.0;
				}
			}
		}

		// go over all triangles and store form factors in tripletList 
		for (int col = 0; col < numtriangles; col++){
			float formfactorRC = hitarray[col] / float(numrays);
			if (formfactorRC > 0.0) {
				tripletList.push_back(Tripl(row, col, formfactorRC));
				//std::cout << "Inserting form factor " << row << "->" << col << " with " << formfactorRC << " at ( " << row << ", " << col << " )" << std::endl;
			}
		}

		numfilled += numtriangles;
		// draw progress bar
		int barWidth = 70;
		float progress = float(float(numfilled) / float(numtriangles*(numtriangles - 1)));
		std::cout << "[";
		int pos = barWidth * progress;
		for (int i = 0; i < barWidth; ++i) {
			if (i < pos) std::cout << "=";
			else if (i == pos) std::cout << ">";
			else std::cout << " ";
		}
		std::cout << "] " << int(progress * 100.0) << " %\r";
		std::cout.flush();
	}

	RadMat.setFromTriplets(tripletList.begin(), tripletList.end());
	std::cout << "... done!                                                                                       " << std::endl;
}

bool OptixPrimeFunctionality::shootPatchRay(std::vector<optix_functionality::Hit> &patches, std::vector<Vertex> &vertices) {
	optix::float3  pointA = triangle_math::uv2xyz(patches[0].triangleId, patches[0].uv, vertices);
	optix::float3  pointB = triangle_math::uv2xyz(patches[1].triangleId, patches[1].uv, vertices);
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> ray = { pointA + optix::normalize(pointB - pointA)*0.000001f, optix::normalize(pointB - pointA) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	optix_functionality::Hit hit;
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

bool OptixPrimeFunctionality::intersectMouse(bool &left, double xpos, double ypos, int optixW, int optixH, Camera &camera, std::vector<std::vector<MatrixIndex>> &trianglesonScreen,
	std::vector<glm::vec3> &optixView, std::vector<optix_functionality::Hit> &patches, std::vector<Vertex> &vertices) {
	bool hitB = true;
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> ray = { camera.eye, optix::normalize(optix::make_float3(xpos + camera.dir.x, ypos + camera.dir.y, camera.dir.z)) };
	query->setRays(1, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, ray.data());
	optix_functionality::Hit hit;
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