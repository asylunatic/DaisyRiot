#include "TriangleMath.h"

optix::float3 TriangleMath::uv2xyz(int triangleId, optix::float2 &uv, std::vector<Vertex> &vertices) {
	glm::vec3 a = vertices[triangleId * 3].pos;
	glm::vec3 b = vertices[triangleId * 3 + 1].pos;
	glm::vec3 c = vertices[triangleId * 3 + 2].pos;
	glm::vec3 point = a + uv.x*(b - a) + uv.y*(c - a);
	return optix::make_float3(point.x, point.y, point.z);
}

glm::vec3 TriangleMath::calculateCentre(std::vector<glm::vec3> triangle) {
	glm::vec3 centre = (triangle[0] + triangle[1] + triangle[2]);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

glm::vec3 TriangleMath::calculateCentre(float triangleId, std::vector<Vertex> &vertices) {
	glm::vec3 centre = (vertices[triangleId * 3].pos + vertices[triangleId * 3 + 1].pos + vertices[triangleId * 3 + 2].pos);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

glm::vec3 TriangleMath::avgNormal(float triangleId, std::vector<Vertex> &vertices) {
	glm::vec3 avg = (vertices[triangleId * 3].normal + vertices[triangleId * 3 + 1].normal + vertices[triangleId * 3 + 2].normal);
	avg = glm::vec3(avg.x / 3, avg.y / 3, avg.z / 3);
	return glm::normalize(avg);
}

float TriangleMath::calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	return 0.5*glm::length(glm::cross(ab, ac));
}

float TriangleMath::calculateSurface(std::vector<glm::vec3> triangle) {
	return TriangleMath::calculateSurface(triangle[0], triangle[1], triangle[2]);
}


float TriangleMath::calcPointFormfactor(Vertex orig, Vertex dest, float surface) {
	float formfactor = 0;
	float dot1 = glm::dot(orig.normal, glm::normalize(dest.pos - orig.pos));
	float dot2 = glm::dot(dest.normal, glm::normalize(orig.pos - dest.pos));
	if (dot1 > 0 && dot2 > 0) {
		float length = glm::length(dest.pos - orig.pos);
		// dot product of normalized vectors gives 1*1*cos(theta) 
		// so dot1 = cos(theta) i
		// dot2 = cos(theta) j
		formfactor = ((dot1 * dot2) / (std::powf(length, 2)*M_PIf)) * surface;
	}
	return formfactor;
}

std::vector<std::vector<glm::vec3>> TriangleMath::divideInFourTriangles(float triangleId, std::vector<Vertex> &vertices) {
	glm::vec3 a = vertices[triangleId * 3].pos;
	glm::vec3 b = vertices[triangleId * 3 + 1].pos;
	glm::vec3 c = vertices[triangleId * 3 + 2].pos;
	std::vector<glm::vec3> innerTriangle;
	glm::vec3 innerA = ((b - a) / 2.0f) + a;
	glm::vec3 innerC = ((c - a) / 2.0f) + a;
	glm::vec3 innerB = ((b - c) / 2.0f) + c;

	std::vector<std::vector<glm::vec3>> res{
		std::vector<glm::vec3>{a, innerC, innerA},
		std::vector<glm::vec3>{innerC, c, innerB},
		std::vector<glm::vec3>{innerA, innerB, b},
		std::vector<glm::vec3>{innerA, innerB, innerC} };
	return res;
}

bool TriangleMath::isFacingBack(glm::vec3 origin, float destPatch, std::vector<Vertex> &vertices) {
	// check if facing back of triangle:
	glm::vec3 center_dest = TriangleMath::calculateCentre(destPatch, vertices);
	glm::vec3 normal_dest = TriangleMath::avgNormal(destPatch, vertices);

	glm::vec3 desty = glm::normalize(center_dest - origin);
	if (glm::dot(desty, normal_dest) >= 0){
		return true;
	}
	else return false;
}