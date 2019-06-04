#pragma once

#include <glm/glm.hpp>


class Material{
public:
	glm::vec3 rgbcolor; // will treat as if constains wavelengths of 400 500 and 650 nm (rougly rgb)
	glm::vec3 xyzcolor;
	glm::vec3 emission;
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission) :rgbcolor(rgbcolor), emission(emission){
	};
	
	Material(){
		rgbcolor = glm::vec3();
		xyzcolor = glm::vec3();
		emission = glm::vec3();
	};

	// debug function, aught to return same color as rgb value as this is a linear conversion
	glm::vec3 get_double_converted_color(){
		glm::vec3 xyz;
		RGBToXYZ(xyz, rgbcolor);
		glm::vec3 rgb;
		XYZToRGB(xyz, rgb);
		return rgb;
	}

private:
	// color conversion according to pbrt
	inline void XYZToRGB(glm::vec3 &xyz, glm::vec3 &rgb) {
		rgb[0] = 3.240479f * xyz[0] - 1.537150f * xyz[1] - 0.498535f * xyz[2];
		rgb[1] = -0.969256f * xyz[0] + 1.875991f * xyz[1] + 0.041556f * xyz[2];
		rgb[2] = 0.055648f * xyz[0] - 0.204043f * xyz[1] + 1.057311f * xyz[2];
	}

	inline void RGBToXYZ(glm::vec3 &xyz, glm::vec3 &rgb) {
		xyz[0] = 0.412453f * rgb[0] + 0.357580f * rgb[1] + 0.180423f * rgb[2];
		xyz[1] = 0.212671f * rgb[0] + 0.715160f * rgb[1] + 0.072169f * rgb[2];
		xyz[2] = 0.019334f * rgb[0] + 0.119193f * rgb[1] + 0.950227f * rgb[2];
	}
};
