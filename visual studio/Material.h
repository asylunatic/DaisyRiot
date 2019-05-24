#pragma once

#include <glm/glm.hpp>

class Material{
public:
	glm::vec3 rgbcolor;
	glm::vec3 emission;
	Material(glm::vec3 rgbcolor, glm::vec3 emission) :rgbcolor(rgbcolor), emission(emission){};
	Material(){
		rgbcolor = glm::vec3();
		emission = glm::vec3();
	}
};
