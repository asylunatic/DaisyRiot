#ifndef __CUDACC__
	#define __CUDACC__
#endif

#include <optix_world.h>
#include <optix.h>

using namespace optix;

rtDeclareVariable(float2, mousePos, , );
rtDeclareVariable(rtObject, top_object, , );


RT_PROGRAM void raytraceExecution() 
{  
	 rtPrintf("THIS. IS. CUDAAAAAAAAA! %f %f", mousePos.x, mousePos.y);
	 float3 origin = make_float3(mousePos.x, mousePos.y, 1.5);
	 float3 direction = make_float3(0,0, -1);
	 optix::Ray ray(origin, direction, 0, 0, RT_DEFAULT_MAX);
	 bool hit = false;

	 rtTrace(top_object, ray, hit);

} 