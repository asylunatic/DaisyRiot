# DaisyRiot
Project for a global illumination renderer, using radiosity.

## Prerequisites
Cuda version 9.0
Optix SDK version 4.1.1 (https://developer.nvidia.com/designworks/optix/downloads/legacy)

## Installation
Set path variable: C:\ProgramData\NVIDIA Corporation\OptiX SDK 4.1.1\bin64

And two unfortunate hacky adjustments:

1) In optix_prime.h, linenumber 875: 
RTPresult RTPAPI rtpQuerySetCudaStream(RTPquery query, cudaStream_t stream);
Change this line to:
RTPresult RTPAPI rtpQuerySetCudaStream(RTPquery query, optix::cudaStream_t stream);

2) In Ref.h, move the following two method declarations to the public section: 

/// Increment the reference count

virtual unsigned int ref();


/// Decrement the reference count

virtual unsigned int unref();
