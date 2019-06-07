#pragma once

#include <glm/glm.hpp>
#include "rgb2spec.h"


class Material{
public:
	glm::vec3 rgbcolor; // will treat as if constains wavelengths of 400 500 and 650 nm (rougly rgb)
	glm::vec3 rgbspectralcolor;
	glm::vec3 xyzcolor;
	glm::vec3 emission;
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission) :rgbcolor(rgbcolor), emission(emission){
		// set spectralcolorfor rgb
		RGB2Spec *model = rgb2spec_load("tables/srgb.coeff");
		float rgb[3] = { rgbcolor.x, rgbcolor.y, rgbcolor.z }, coeff[3];
		rgb2spec_fetch(model, rgb, coeff);

		float sample_600 = rgb2spec_eval_precise(coeff, 600);
		float sample_550 = rgb2spec_eval_precise(coeff, 550);
		float sample_450 = rgb2spec_eval_precise(coeff, 450);

		rgbspectralcolor = { sample_600, sample_550, sample_450 };

		rgb2spec_free(model);
	};
	
	Material(){
		rgbcolor = glm::vec3();
		xyzcolor = glm::vec3();
		emission = glm::vec3();
	};

	glm::vec3 get_color_from_spectrum(){
		glm::vec3 xyz = cie1931WavelengthToXYZFit(600) * rgbspectralcolor[0]
						+ cie1931WavelengthToXYZFit(550) * rgbspectralcolor[1]
						+ cie1931WavelengthToXYZFit(460) * rgbspectralcolor[2];
		glm::vec3 rgb;
		XYZToRGB(xyz, rgb);
		return rgb;
	}

private:
	/**
	* A multi-lobe, piecewise Gaussian fit of CIE 1931 XYZ Color Matching Functions by Wyman el al. from Nvidia. The
	* code here is adopted from the Listing 1 of the paper authored by Wyman et al.
	* <p>
	* Reference: Chris Wyman, Peter-Pike Sloan, and Peter Shirley, Simple Analytic Approximations to the CIE XYZ Color
	* Matching Functions, Journal of Computer Graphics Techniques (JCGT), vol. 2, no. 2, 1-11, 2013.
	*
	* @param wavelength wavelength in nm
	* @return XYZ in a double array in the order of X, Y, Z. each value in the range of [0.0, 1.0]
	*/
	glm::vec3 cie1931WavelengthToXYZFit(double wavelength) {
		double x, y, z;
		double wave = wavelength;

		{
			double t1 = (wave - 442.0) * ((wave < 442.0) ? 0.0624 : 0.0374);
			double t2 = (wave - 599.8) * ((wave < 599.8) ? 0.0264 : 0.0323);
			double t3 = (wave - 501.1) * ((wave < 501.1) ? 0.0490 : 0.0382);

			x = 0.362 * std::exp(-0.5 * t1 * t1)
				+ 1.056 * std::exp(-0.5 * t2 * t2)
				- 0.065 * std::exp(-0.5 * t3 * t3);
		}

		{
			double t1 = (wave - 568.8) * ((wave < 568.8) ? 0.0213 : 0.0247);
			double t2 = (wave - 530.9) * ((wave < 530.9) ? 0.0613 : 0.0322);

			y = 0.821 * std::exp(-0.5 * t1 * t1)
				+ 0.286 * std::exp(-0.5 * t2 * t2);
		}

		{
			double t1 = (wave - 437.0) * ((wave < 437.0) ? 0.0845 : 0.0278);
			double t2 = (wave - 459.0) * ((wave < 459.0) ? 0.0385 : 0.0725);

			z = 1.217 * std::exp(-0.5 * t1 * t1)
				+ 0.681 * std::exp(-0.5 * t2 * t2);
		}

		return{ x, y, z };
	}

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
