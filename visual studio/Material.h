#pragma once

#include <glm/glm.hpp>


class Material{
public:
	glm::vec3 rgbcolor;
	glm::vec3 xyzcolor;
	glm::vec3 emission;
	glm::mat3x3 xyz2srgb;
	glm::mat3x3 srgb2xyz;
	glm::mat3x4 wave2xyz;

	glm::vec4 wavebins;// = { 0.f, 0.f, 0.f, 0.f }; // contains wavelengths of respectively 456.4, 490.9, 557.7 and 631.4 nm
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission) :rgbcolor(rgbcolor), emission(emission){
		xyz2srgb = xyz_to_srgb();
		srgb2xyz = srgb_to_xyz();
		/*wave2xyz = { 0.1986f, -0.0569f, 0.4934f, 0.4228f,
					-0.0034f, 0.1856f, 0.6770f, 0.1998f,
					0.9632f, 0.0931f, 0.0806f, -0.0791f };*/
		// col major definition?
		wave2xyz = { 0.1986f, -0.0034f, 0.9632f, 
					-0.0569f, 0.1856f, 0.0931f, 
					0.4934f, 0.6770f, 0.0806f, 
					0.4228f, 0.1998f, -0.0791f };
	};
	
	Material(){
		rgbcolor = glm::vec3();
		xyzcolor = glm::vec3();
		emission = glm::vec3();
		xyz2srgb = xyz_to_srgb();
		srgb2xyz = srgb_to_xyz();
		/*wave2xyz = { 0.1986f, -0.0569f, 0.4934f, 0.4228f,
		-0.0034f, 0.1856f, 0.6770f, 0.1998f,
		0.9632f, 0.0931f, 0.0806f, -0.0791f };*/
		// col major definition?
		wave2xyz = { 0.1986f, -0.0034f, 0.9632f,
					-0.0569f, 0.1856f, 0.0931f,
					0.4934f, 0.6770f, 0.0806f,
					0.4228f, 0.1998f, -0.0791f };
	};

	// debug function, aught to return same color as rgb value as this is a linear conversion
	glm::vec3 get_double_converted_color(){
		glm::vec3 xyz = rgbcolor * srgb2xyz;
		glm::vec3 srgb = xyz * xyz2srgb;
		return srgb;
	}

	// debug function, aught to return same color as rgb value as this is a linear conversion
	glm::vec3 get_double_triple_converted_color(){
		glm::vec3 xyz = rgbcolor * srgb2xyz;
		// convert to wavelengths
		glm::mat3x4 xyz2wave = glm::transpose(wave2xyz);
		glm::vec4 wave = xyz2wave * xyz;
		xyz = wave2xyz * wave;
		glm::vec3 srgb = xyz * xyz2srgb;
		return srgb;
	}

private:
	glm::vec3 wavelength2xyz(glm::vec4 waves){
		return waves * wave2xyz;
	}

	glm::mat3x3 xyz_to_srgb(){
		return glm::inverse(srgb_to_xyz());
	}

	glm::mat3x3 srgb_to_xyz(){
		glm::vec2 rxy(0.64f, 0.33f);
		glm::vec2 gxy(0.3f, 0.6f);
		glm::vec2 bxy(0.15f, 0.06f);
		glm::vec2 wxy(0.3127f, 0.329f);
		return rgb_to_xyz(rxy, gxy, bxy, wxy);
	}

	// color conversion, eagerly stolen from
	// http://www.ryanjuckett.com/programming/rgb-color-space-conversion/
	glm::mat3x3 rgb_to_xyz(const glm::vec2 &red_xy, const glm::vec2 &green_xy, const glm::vec2 &blue_xy, const glm::vec2 & white_xy){
		// generate xyz chromaticity coordinates (x + y + z = 1) from xy coordinates
		glm::vec3 r = { red_xy.x, red_xy.y, 1.0f - (red_xy.x + red_xy.y) };
		glm::vec3 g = { green_xy.x, green_xy.y, 1.0f - (green_xy.x + green_xy.y) };
		glm::vec3 b = { blue_xy.x, blue_xy.y, 1.0f - (blue_xy.x + blue_xy.y) };
		glm::vec3 w = { white_xy.x, white_xy.y, 1.0f - (white_xy.x + white_xy.y) };

		// Convert white xyz coordinate to XYZ coordinate by letting that the white
		// point have and XYZ relative luminance of 1.0. Relative luminance is the Y
		// component of and XYZ color.
		//   XYZ = xyz * (Y / y) 
		w.x /= white_xy.y;
		w.y /= white_xy.y;
		w.z /= white_xy.y;

		// Solve for the transformation matrix 'M' from RGB to XYZ
		glm::mat3x3 pOutput = glm::mat3x3(r, g, b);

		glm::mat3x3 invMat = glm::inverse(pOutput);

		glm::vec3 scale = invMat * w;

		pOutput[0][0] *= scale.x;
		pOutput[1][0] *= scale.x;
		pOutput[2][0] *= scale.x;

		pOutput[0][1] *= scale.y;
		pOutput[1][1] *= scale.y;
		pOutput[2][1] *= scale.y;

		pOutput[0][2] *= scale.z;
		pOutput[1][2] *= scale.z;
		pOutput[2][2] *= scale.z;

		return pOutput;
	}
};
