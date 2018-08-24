#include "ImageExporter.h"
#include "lodepng.h"



ImageExporter::ImageExporter(int width, int height)
{
	encodepixels = new GLubyte[width*height * 3];
}

char* ImageExporter::findFilename(const char* filename, const char* extension){
	std::string temp = filename;
	temp.append(extension);

	// decide upon suitabe outputname:
	int i = 0;
	while (ImageExporter::exists(temp)) {
		temp = filename;
		temp.append(std::to_string(i));
		temp.append(extension);
		i++;
	}
	// decide upon name
	char* name = new char[temp.length() + 1];
	strcpy(name, temp.c_str());
	return name;
}

/*
Encode from raw pixels to disk with a single function call
The image argument has width * height RGBA pixels or width * height * 4 bytes
*/
void ImageExporter::encodeOneStep(const char* filename, const char* extension, unsigned width, unsigned height)
{

	// find suitabe outputname
	char* name = findFilename(filename, extension);

	/*Encode the image*/
	unsigned error = lodepng_encode24_file(name, encodepixels, width, height);

	/*if there's an error, display it*/
	if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
}


ImageExporter::~ImageExporter()
{
}
