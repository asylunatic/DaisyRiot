# DaisyRiot
Project for a global illumination rediosity renderer that supports fluorescent materials.

## Prerequisites
Cuda version 9.0 (https://developer.nvidia.com/cuda-toolkit-archive)   
Optix SDK version 4.1.1 (https://developer.nvidia.com/designworks/optix/downloads/legacy)   
Visual Studio 2013 is recommended, the provided project files are for VS2013. Later versions may work, but no guarantees on that.

## Installation
Set path variable: C:\ProgramData\NVIDIA Corporation\OptiX SDK 4.1.1\bin64

Two unfortunate hacky adjustments:

1) In optix_prime.h, find line number 875:   
`RTPresult RTPAPI rtpQuerySetCudaStream(RTPquery query, cudaStream_t stream);`  
Change this line to:   
`RTPresult RTPAPI rtpQuerySetCudaStream(RTPquery query, optix::cudaStream_t stream);`

2) In Ref.h, move the following two method declarations to the public section: 
```
/// Increment the reference count

virtual unsigned int ref();

/// Decrement the reference count

virtual unsigned int unref();
```

## Running DaisyRiot

### Config
Make a copy of the config_example.ini file, rename to config.ini and modify as you see fit. 

### Interaction
A manual of all the options is printed to the console on startup.

### Custom scenes & materials
Materials are best defined in Blender: 
* Diffuse color will be used as the emissive and reflective color. Emissiveness is set with the emit factor under 'Shading'.
* Specular color is interpreted as the fluorescent color of a material. Setting the intensity to zero (or the specular color to black) disables fluorescence for the material.
* Black light sources are defined by setting the material to be completely black and giving it an emissive value. However, when exporting the material, Blender will set the emissiveness to zero. So to enfore this, the emissiveness of the black material has to be added by manually editing the materials file: update the Ke values for the blacklight material to the appropriate value (in between 0-2).
