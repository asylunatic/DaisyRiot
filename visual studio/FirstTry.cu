#ifndef __CUDACC__
	#define __CUDACC__
#endif

#include <optix_world.h>
#include <optix.h>

using namespace optix;

RT_PROGRAM void raytraceExecution() 
{  
	 rtPrintf("THIS. IS. CUDAAAAAAAAA!");
} 