#include "parallellism.cuh"

__global__
void parallellism::add(int n, float *x, float *y)
{
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;
	for (int i = index; i < n; i += stride)
		y[i] = x[i] + y[i];
}

void parallellism::runAdd() {
	int N = 1 << 20;
	float *x, *y;

	// Allocate Unified Memory � accessible from CPU or GPU
	cudaMallocManaged(&x, N*sizeof(float));
	cudaMallocManaged(&y, N*sizeof(float));

	// initialize x and y arrays on the host
	for (int i = 0; i < N; i++) {
		x[i] = 1.0f;
		y[i] = 2.0f;
	}

	// Run kernel on 1M elements on the GPU
	int blockSize = 256;
	int numBlocks = (N + blockSize - 1) / blockSize;
	add <<<numBlocks, blockSize >>>(N, x, y);

	// Wait for GPU to finish before accessing on host
	cudaDeviceSynchronize();

	// Check for errors (all values should be 3.0f)
	float maxError = 0.0f;
	for (int i = 0; i < N; i++)
		maxError = fmax(maxError, fabs(y[i] - 3.0f));
	std::cout << "Max error: " << maxError << std::endl;

	// Free memory
	cudaFree(x);
	cudaFree(y);
}

std::vector<parallellism::Tripl> parallellism::runCalculateRadiosityMatrix(vertex::MeshS& mesh) {
	std::vector<Tripl> tripletList = {};
	int numtriangles = mesh.triangleIndices.size();

	//Load mesh into cuda memory
	glm::vec3* vertices;// = new glm::vec3[mesh.vertices.size()];
	glm::vec3* normals;// = new glm::vec3[mesh.normals.size()];
	vertex::TriangleIndex* triangleIndices;// = new vertex::TriangleIndex[mesh.triangleIndices.size()];
	cudaMallocManaged(&vertices, mesh.vertices.size()*sizeof(glm::vec3));
	cudaMallocManaged(&normals, mesh.normals.size()*sizeof(glm::vec3));
	cudaMallocManaged(&triangleIndices, mesh.triangleIndices.size()*sizeof(vertex::TriangleIndex));
	cudaCheckError();
	std::copy(std::begin(mesh.vertices), std::end(mesh.vertices), vertices);
	std::copy(std::begin(mesh.normals), std::end(mesh.normals), normals);
	std::copy(mesh.triangleIndices.begin(), mesh.triangleIndices.end(), triangleIndices);


	int numfilled = 0;
	

	for (int row = 0; row < numtriangles; row++) {
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
		Tripl* tripletRow;// = new Tripl[numtriangles];
		cudaMallocManaged(&tripletRow, numtriangles*sizeof(Tripl));
		cudaCheckError();

		calculateRow<<<1, 256>>>(row, tripletRow, vertices, normals, triangleIndices, numtriangles);
		cudaDeviceSynchronize();
		cudaCheckError();

		tripletList.insert(tripletList.end(), tripletRow, &tripletRow[numtriangles]);

		// draw progress bar
		numfilled += numtriangles;
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

	//for (int i = 0; i < tripletList.size(); i++) {
	//	std::cout << "Form factor at index " << i << " with index (" << tripletList[i].m_row << " ,  " << tripletList[i].m_col << " is : " << tripletList[i].m_value << std::endl;
	//}
	return tripletList;
}

__global__
void parallellism::calculateRow(int row, Tripl* rowTripletList,
glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices, int numtriangles) {
	int threadIndex =  threadIdx.x;
	int stride = blockDim.x;
	for (int col = threadIndex; col < numtriangles; col += stride) {
		float formfactorRC = p2pFormfactor(row, col, vertices, normals, triangleIndices);
		if (formfactorRC > 0.0) {
			// at place (x, y) we want the form factor y->x 
			// but as this is a col major matrix we store (x, y) at (y, x) -> confused yet?
			rowTripletList[col] = { row, col, formfactorRC };
		}
		else {
			rowTripletList[col] = { row, col, 0.0 };
		}
	}
}

__device__
float parallellism::p2pFormfactor(int originPatch, int destPatch,
glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices) {
	// subdivide triangles and return in vector w/ 12 entries (4*3 coordinates)

	glm::vec3 origintriangles[4][3];
	glm::vec3 destinationtriangles[4][3];
	divideInFourTriangles(origintriangles, originPatch, vertices, normals, triangleIndices);
	divideInFourTriangles(destinationtriangles, destPatch, vertices, normals, triangleIndices);

	// init vectors
	glm::vec3 originpoints[4];
	glm::vec3 destinationpoints[4];

	glm::vec3 originNormal = avgNormal(originPatch, vertices, normals, triangleIndices);
	glm::vec3 destNormal = avgNormal(destPatch, vertices, normals, triangleIndices);

	// calculate centers of subdivided triangles
	for (int i = 0; i < 4; i++) {
		originpoints[i] = calculateCentre(origintriangles[i]);
		destinationpoints[i] = calculateCentre(destinationtriangles[i]);
	}
	

	float formfactor = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			vertex::Vertex arg1 = { originpoints[i], originNormal };
			vertex::Vertex arg2 = { destinationpoints[j], destNormal };
			float surface = calculateSurface(origintriangles[i])*calculateSurface(destinationtriangles[j]);
			formfactor = formfactor + calcPointFormfactor(arg1, arg2, surface);
		}
	}

	formfactor = formfactor / calculateSurface(originPatch, vertices, normals, triangleIndices);

	return formfactor;

}

__device__
void parallellism::divideInFourTriangles(glm::vec3 res[4][3], int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices) {
	glm::vec3 a = vertices[triangleIndices[triangleId].vertex.x];
	glm::vec3 b = vertices[triangleIndices[triangleId].vertex.y];
	glm::vec3 c = vertices[triangleIndices[triangleId].vertex.z];
	glm::vec3 innerA = ((b - a) / 2.0f) + a;
	glm::vec3 innerC = ((c - a) / 2.0f) + a;
	glm::vec3 innerB = ((b - c) / 2.0f) + c;

	glm::vec3 temp[4][3] = { { a, innerC, innerA },
	{ innerC, c, innerB },
	{ innerA, innerB, b },
	{ innerA, innerB, innerC } };

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 3; j++) {
			res[i][j] = temp[i][j];
		}
	}
}

__device__
glm::vec3 parallellism::calculateCentre(int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices) {
	glm::vec3 centre = (vertices[triangleIndices[triangleId].vertex.x] +
		vertices[triangleIndices[triangleId].vertex.y] +
		vertices[triangleIndices[triangleId].vertex.z]);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

__device__
glm::vec3 parallellism::calculateCentre(glm::vec3* triangle) {
	glm::vec3 centre = (triangle[0] + triangle[1] + triangle[2]);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

__device__
glm::vec3 parallellism::avgNormal(int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices) {
	glm::vec3 avg = (normals[triangleIndices[triangleId].normal.x] +
		normals[triangleIndices[triangleId].normal.y] +
		normals[triangleIndices[triangleId].normal.z]);
	avg = glm::vec3(avg.x / 3, avg.y / 3, avg.z / 3);
	return glm::normalize(avg);
}

__device__
float parallellism::calcPointFormfactor(vertex::Vertex orig, vertex::Vertex dest, float surface) {
	float formfactor = 0;
	float dot1 = glm::dot(orig.normal, glm::normalize(dest.pos - orig.pos));
	float dot2 = glm::dot(dest.normal, glm::normalize(orig.pos - dest.pos));
	if (dot1 > 0 && dot2 > 0) {
		float length = glm::length(dest.pos - orig.pos);
		formfactor = ((dot1 * dot2) / (std::powf(length, 2)*CUDART_PI)) * surface;
	}
	return formfactor;
}

__device__
float parallellism::calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	return 0.5*glm::length(glm::cross(ab, ac));
}

__device__
float parallellism::calculateSurface(glm::vec3* triangle) {
	return calculateSurface(triangle[0], triangle[1], triangle[2]);
}

__device__
float parallellism::calculateSurface(int triangleId, glm::vec3* vertices, glm::vec3* normals, vertex::TriangleIndex* triangleIndices) {
	return calculateSurface(
		vertices[triangleIndices[triangleId].vertex.x],
		vertices[triangleIndices[triangleId].vertex.y],
		vertices[triangleIndices[triangleId].vertex.z]);
}
