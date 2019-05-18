#include "OptixPrimeFunctionality.h"

void OptixPrimeFunctionality::initOptixPrime(vertex::MeshS& mesh) {
	contextP = optix::prime::Context::create(RTP_CONTEXT_TYPE_CUDA);
	optix::prime::BufferDesc vertexBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_VERTEX_FLOAT3, RTP_BUFFER_TYPE_HOST, mesh.vertices.data());
	vertexBuffer->setRange(0, mesh.vertices.size());
	optix::prime::BufferDesc indexBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_INDICES_INT3, RTP_BUFFER_TYPE_HOST, mesh.triangleIndices.data());
	indexBuffer->setRange(0, mesh.triangleIndices.size());
	indexBuffer->setStride(sizeof(vertex::TriangleIndex));
	model = contextP->createModel();
	model->setTriangles(indexBuffer, vertexBuffer);
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

void  OptixPrimeFunctionality::doOptixPrime(int width, int height, std::vector<glm::vec3> &optixView,
Camera &camera, std::vector<std::vector<MatrixIndex>> &trianglesonScreen, vertex::MeshS& mesh) {

	std::vector<optix_functionality::Hit> hits;
	hits.resize(width*height);
	optixView.resize(width*height);
	optix::float3 upperLeftCorner = camera.debug_marijn_eye + camera.debug_marijn_dir + optix::make_float3(-1.0f, 1.0f, 0.0f);
	optix::prime::Query query = model->createQuery(RTP_QUERY_TYPE_CLOSEST);
	std::vector<optix::float3> rays;
	rays.resize(width*height * 2);

	//// generate rays the marijn way
	//for (size_t j = 0; j < optixH; j++) {
	//	for (size_t i = 0; i < optixW; i++) {
	//		rays[(j*optixH + i) * 2] = camera.debug_marijn_eye;
	//		rays[((j*optixH + i) * 2) + 1] = optix::normalize(upperLeftCorner + optix::make_float3(i*2.0f / optixW, -1.0f*(j*2.0f / optixH), 0) - camera.debug_marijn_eye);
	//	}
	//}

	// generate rays the un_project way
	glm::mat4x4 lookat = glm::lookAt(optix_functionality::optix2glmf3(camera.eye), optix_functionality::optix2glmf3(camera.dir), optix_functionality::optix2glmf3(camera.up));
	//glm::mat4x4 projection = glm::ortho(-10.0, 10.0, -10.0, 10.0);
	glm::mat4x4 projection = glm::perspective(45.0f, (float)(800) / (float)(600), 0.1f, 1000.0f);
	for (size_t x = 0; x < width; x++) {
		for (size_t y = 0; y < height; y++) {
			glm::vec3 win(x, y, 0.0);
			glm::vec3 world_coord = glm::unProject(win, lookat, projection, camera.viewport);
			if (x % 10 == 0 && y % 10 == 0){
				//std::cout << "unprojected (x " << x << " y " << y << ") = " << world_coord.x << " " << world_coord.y << " " << world_coord.z << std::endl;
			}
			rays[(y*width + x) * 2] = optix_functionality::glm2optixf3(world_coord);

			glm::vec3 win_dir(x, y, 1.0);
			glm::vec3 dir_coord = glm::unProject(win_dir, lookat, projection, camera.viewport);
			//optix::float3 dir = (camera.dir - camera.eye);
			//dir = normalize(dir);
			rays[((y*width + x) * 2) + 1] = optix_functionality::glm2optixf3(dir_coord); //dir;
		}
	}

	query->setRays(width*height, RTP_BUFFER_FORMAT_RAY_ORIGIN_DIRECTION, RTP_BUFFER_TYPE_HOST, rays.data());
	optix::prime::BufferDesc hitBuffer = contextP->createBufferDesc(RTP_BUFFER_FORMAT_HIT_T_TRIID_U_V, RTP_BUFFER_TYPE_HOST, hits.data());
	hitBuffer->setRange(0, width*height);
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
	trianglesonScreen.resize(mesh.triangleIndices.size());

	for (size_t x = 0; x < width; x++) {
		for (size_t y = 0; y < height; y++) {
			int pixelIndex = y*width + x;
			optixView[pixelIndex] = (hits[pixelIndex].t > 0) ? glm::vec3(glm::abs(mesh.normals[mesh.triangleIndices[hits[pixelIndex].triangleId].normal.x])) : glm::vec3(0.0f, 0.0f, 0.0f);
			if (hits[pixelIndex].t > 0 
				&& !triangle_math::isFacingBack(optix_functionality::optix2glmf3(camera.eye), hits[pixelIndex].triangleId, mesh)
				) {
				MatrixIndex index = {};
				index.col = x;
				index.row = y;
				index.uv = {hits[pixelIndex].uv.x, hits[pixelIndex].uv.y};
				trianglesonScreen[hits[pixelIndex].triangleId].push_back(index);
			}
		}
	}

}

float OptixPrimeFunctionality::p2pFormfactor(int originPatch, int destPatch, vertex::MeshS& mesh, std::vector<UV> &rands) {	
	// subdivide triangles and return in vector w/ 12 entries (4*3 coordinates)
	std::vector<std::vector<glm::vec3>> origintriangles, destinationtriangles;
	origintriangles = triangle_math::divideInFourTriangles(originPatch, mesh);
	destinationtriangles = triangle_math::divideInFourTriangles(destPatch, mesh);

	// init vectors
	std::vector<glm::vec3> originpoints, destinationpoints;
	originpoints.resize(4);
	destinationpoints.resize(4);
	
	glm::vec3 originNormal = triangle_math::avgNormal(originPatch, mesh);
	glm::vec3 destNormal = triangle_math::avgNormal(destPatch, mesh);

	// calculate centers of subdivided triangles
	for (int i = 0; i < 4; i++) {
		originpoints[i] = triangle_math::calculateCentre(origintriangles[i]);
		destinationpoints[i] = triangle_math::calculateCentre(destinationtriangles[i]);
	}

	float formfactor = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			formfactor = formfactor + triangle_math::calcPointFormfactor({ originpoints[i], originNormal },
				{ destinationpoints[j], destNormal },
				triangle_math::calculateSurface(origintriangles[i])*triangle_math::calculateSurface(destinationtriangles[i]));
		}
	}
	formfactor = formfactor / triangle_math::calculateSurface(originPatch, mesh);
	
	float visibility = OptixPrimeFunctionality::calculateVisibility(originPatch, destPatch, mesh, contextP, model, rands);

	return formfactor * visibility;

}

float OptixPrimeFunctionality::calculateVisibility(int originPatch, int destPatch, vertex::MeshS& mesh, optix::prime::Context &contextP, optix::prime::Model &model, std::vector<UV> &rands) {
	std::vector<optix::float3> rays;
	rays.resize(2 * RAYS_PER_PATCH);

	std::vector<optix_functionality::Hit> hits;
	hits.resize(RAYS_PER_PATCH);

	optix::float3 origin;
	optix::float3 dest;
	for (int i = 0; i < RAYS_PER_PATCH; i++) {
		origin = triangle_math::uv2xyz(originPatch, optix::make_float2(rands[i].u, rands[i].v), mesh);
		dest = triangle_math::uv2xyz(destPatch, optix::make_float2(rands[i].u, rands[i].v), mesh);
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

float OptixPrimeFunctionality::p2pFormfactorNusselt(int originPatch, int destPatch, vertex::MeshS& mesh, std::vector<UV> &rands) {

	glm::vec3 center_origin = triangle_math::calculateCentre(originPatch, mesh);
	glm::vec3 center_dest = triangle_math::calculateCentre(destPatch, mesh);

	glm::vec3 normal_origin = triangle_math::avgNormal(originPatch, mesh);

	if (triangle_math::isFacingBack(center_origin, destPatch, mesh),
		triangle_math::isFacingBack(center_dest, originPatch, mesh)) {
		return 0.0;
	}

	// init vectors to project triangle
	std::vector<glm::vec3> hemitriangle;
	std::vector<glm::vec3> projtriangle;
	projtriangle.resize(3);
	hemitriangle.resize(3);

	for (int i = 0; i < 3; i++) {
		hemitriangle[i] = center_origin + glm::normalize(mesh.vertices[mesh.triangleIndices[destPatch].vertex[i]] - center_origin);

		// calculate the projection of the vertex of the hemi triangle onto the plane that contains the triangle: 
		// (as per https://stackoverflow.com/questions/9605556/how-to-project-a-point-onto-a-plane-in-3d)
		// Given a point-normal definition of a plane with normal n and point o on the plane, a point p', 
		// being the point on the plane closest to the given point p, can be found by:
		// p' = p - (n ⋅ (p - o)) * n
		projtriangle[i] = hemitriangle[i] - (glm::dot(normal_origin, (hemitriangle[i] - center_origin))) * normal_origin;
	}
	
	float visibility = calculateVisibility(originPatch, destPatch, mesh, contextP, model, rands);

	float formfactor = triangle_math::calculateSurface(projtriangle[0], projtriangle[1], projtriangle[2]) / M_PIf;
	float totalformfactor = formfactor * visibility;

	return totalformfactor;

}

// TODO: optimize insertion, this is probably best done with triplets, as explained on:
// https://eigen.tuxfamily.org/dox/group__TutorialSparse.html
void OptixPrimeFunctionality::calculateRadiosityMatrix(SpMat &RadMat, vertex::MeshS& mesh, std::vector<UV> &rands) {
	std::cout << "Calculating radiosity matrix..." << std::endl;
	int numtriangles = mesh.triangleIndices.size();
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
			float formfactorRC = p2pFormfactor(row, col, mesh, rands);
			if (formfactorRC > 0.0) {
				// at place (x, y) we want the form factor y->x 
				// but as this is a col major matrix we store (x, y) at (y, x) -> confused yet?
				tripletList.push_back(Tripl(row, col, formfactorRC));
				//std::cout << "Inserting form factor " << row << "->" << col << " with " << formfactorRC << " at ( " << row << ", " << col << " )" << std::endl;

				// use reprocipity theorem to calculate form factor the other way around
				float formfactorCR = (triangle_math::calculateSurface(row, mesh)*formfactorRC) / triangle_math::calculateSurface(col, mesh);
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

void OptixPrimeFunctionality::calculateRadiosityMatrixStochastic(SpMat &RadMat, vertex::MeshS& mesh, std::vector<UV> &rands) {
	
	// init some shit
	int numtriangles = mesh.triangleIndices.size();
	int numrays = 1000;
	std::srand(std::time(nullptr)); // use current time as seed for random generator
	std::vector<Tripl> tripletList;
	
	int numfilled = 0;
	for (int row = 0; row < numtriangles; row++) {
		// retrieve origin and normal of triangle w/ row as index
		optix::float3 origin = optix_functionality::glm2optixf3(triangle_math::calculateCentre(row, mesh));
		glm::vec3 rownormal = triangle_math::avgNormal(row, mesh);

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
			glm::vec3 d = glm::normalize(mesh.vertices[mesh.triangleIndices[row].vertex.x] - mesh.vertices[mesh.triangleIndices[row].vertex.y]);
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
				glm::vec3 destcenter = triangle_math::calculateCentre(hit.triangleId, mesh);
				glm::vec3 destnormal = triangle_math::avgNormal(hit.triangleId, mesh);
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

bool OptixPrimeFunctionality::shootPatchRay(std::vector<optix_functionality::Hit> &patches, vertex::MeshS& mesh) {
	optix::float3  pointA = triangle_math::uv2xyz(patches[0].triangleId, patches[0].uv, mesh);
	optix::float3  pointB = triangle_math::uv2xyz(patches[1].triangleId, patches[1].uv, mesh);
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
	std::vector<glm::vec3> &optixView, std::vector<optix_functionality::Hit> &patches, vertex::MeshS& mesh) {
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
			hitB = shootPatchRay(patches, mesh);
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