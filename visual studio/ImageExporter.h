#pragma once
#include <sys/stat.h>
#include <sstream>
#include <GL/glew.h>


class ImageExporter
{

public:
	ImageExporter(int width, int height);

	static inline bool exists(const std::string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}

	// making these global is a very elegant solution for the stack overflow 
	// that would inevitably follow when declaring it in the function scope where it actually belongs
	GLubyte* encodepixels;

	static char* findFilename(const char* filename, const char* extension);

	void encodeOneStep(const char* filename, const char* extension, unsigned width, unsigned height);

	~ImageExporter();
};

