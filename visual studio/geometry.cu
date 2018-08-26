#ifndef __CUDACC__
	#define __CUDACC__
#endif

#include <optix_world.h>
#include <optix.h>

using namespace optix;

rtDeclareVariable( optix::Ray, ray, rtCurrentRay, );

RT_PROGRAM void intersection(int prim_index) {
float3 ta = make_float3(0, 0, 0);
float3 tb = make_float3(1, 0, 0);
float3 tc = make_float3(0, 1, 0);
float3 normal = make_float3(0, 0, 1);
Aabb triangle = Aabb(ta, tb, tc);

//triangle plane = plane with normal 'normal' and origin 'ta'
float t;
if (dot(ray.direction, normal) != 0) t = dot((ta - ray.origin), normal)/dot(ray.direction, normal);
else t = -1;

float3 p = ray.origin + t * ray.direction; 
if (triangle.contains(p)) {
	rtPrintf("Hit!");
}
}

RT_PROGRAM void boundingbox(int prim_index, float result[6]) {
	float3 ta = make_float3(0, 0, 0);
	float3 tb = make_float3(1, 0, 0);
	float3 tc = make_float3(0, 1, 0);
	float3 normal = make_float3(0, 0, 1);
	Aabb triangle = Aabb(ta, tb, tc);
	result[0] = triangle.m_min.x;
	result[1] = triangle.m_min.y;
	result[2] = triangle.m_min.z;
	result[3] = triangle.m_max.x;
	result[4] = triangle.m_max.y;
	result[5] = triangle.m_max.z;
}