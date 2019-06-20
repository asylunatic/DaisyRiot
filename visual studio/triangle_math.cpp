#include "triangle_math.h"

optix::float3 triangle_math::uv2xyz(int triangleId, optix::float2 &uv, MeshS &mesh) {
	glm::vec3 a = mesh.vertices[mesh.triangleIndices[triangleId].vertex.x];
	glm::vec3 b = mesh.vertices[mesh.triangleIndices[triangleId].vertex.y];
	glm::vec3 c = mesh.vertices[mesh.triangleIndices[triangleId].vertex.z];
	glm::vec3 point = a + uv.x*(b - a) + uv.y*(c - a);
	return optix::make_float3(point.x, point.y, point.z);
}

glm::vec3 triangle_math::calculateCentre(std::vector<glm::vec3> triangle) {
	glm::vec3 centre = (triangle[0] + triangle[1] + triangle[2]);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

glm::vec3 triangle_math::calculateCentre(float triangleId, MeshS &mesh) {
	glm::vec3 centre = (mesh.vertices[mesh.triangleIndices[triangleId].vertex.x] +
		mesh.vertices[mesh.triangleIndices[triangleId].vertex.y] + 
		mesh.vertices[mesh.triangleIndices[triangleId].vertex.z]);
	return glm::vec3(centre.x / 3, centre.y / 3, centre.z / 3);
}

glm::vec3 triangle_math::avgNormal(float triangleId, MeshS &mesh) {
	glm::vec3 avg = (mesh.normals[mesh.triangleIndices[triangleId].normal.x] +
		mesh.normals[mesh.triangleIndices[triangleId].normal.y] + 
		mesh.normals[mesh.triangleIndices[triangleId].normal.z]);
	avg = glm::vec3(avg.x / 3, avg.y / 3, avg.z / 3);
	return glm::normalize(avg);
}

float triangle_math::calculateSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	return 0.5*glm::length(glm::cross(ab, ac));
}

float triangle_math::calculateSurface(std::vector<glm::vec3> triangle) {
	return triangle_math::calculateSurface(triangle[0], triangle[1], triangle[2]);
}

float triangle_math::calculateSurface(float triangleId, MeshS &mesh) {
	return triangle_math::calculateSurface(
		mesh.vertices[mesh.triangleIndices[triangleId].vertex.x], 
		mesh.vertices[mesh.triangleIndices[triangleId].vertex.y], 
		mesh.vertices[mesh.triangleIndices[triangleId].vertex.z]);
}


float triangle_math::calcPointFormfactor(vertex::Vertex orig, vertex::Vertex dest, float surface) {
	float formfactor = 0;
	float dot1 = glm::dot(orig.normal, glm::normalize(dest.pos - orig.pos));
	float dot2 = glm::dot(dest.normal, glm::normalize(orig.pos - dest.pos));
	if (dot1 > 0 && dot2 > 0) {
		float length = glm::length(dest.pos - orig.pos);
		formfactor = ((dot1 * dot2) / (std::powf(length, 2)*M_PIf)) * surface;
	}
	return formfactor;
}

std::vector<std::vector<glm::vec3>> triangle_math::divideInFourTriangles(float triangleId, MeshS &mesh) {
	glm::vec3 a = mesh.vertices[mesh.triangleIndices[triangleId].vertex.x];
	glm::vec3 b = mesh.vertices[mesh.triangleIndices[triangleId].vertex.y];
	glm::vec3 c = mesh.vertices[mesh.triangleIndices[triangleId].vertex.z];
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

bool triangle_math::isFacingBack(glm::vec3 origin, float destPatch, MeshS &mesh) {
	// check if facing back of triangle:
	glm::vec3 center_dest = triangle_math::calculateCentre(destPatch, mesh);
	glm::vec3 normal_dest = triangle_math::avgNormal(destPatch, mesh);

	glm::vec3 desty = glm::normalize(center_dest - origin);
	if (glm::dot(desty, normal_dest) >= 0){
		return true;
	}
	else return false;
}