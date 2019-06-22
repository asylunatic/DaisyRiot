#pragma once

#include <glm/glm.hpp>
#include "rgb2spec.h"
#include "color.h"
#include <Eigen\Dense>

class Material{
public:
	int numwavelengths;

	glm::vec3 rgbcolor; 
	glm::vec3 emission;
	glm::vec3 blacklight_2_rgb;

	std::vector<float> spectral_values;
	std::vector<float> spectral_emission; 
	std::vector<float> &wavelengths;

	Eigen::MatrixXf M; // matrix to influence wavelengths
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission, glm::vec3 blacklight_2_rgb, std::vector<float> &wave_lengths)
		:rgbcolor(rgbcolor), emission(emission), blacklight_2_rgb(blacklight_2_rgb), wavelengths(wave_lengths){

		numwavelengths = wave_lengths.size();
		M = Eigen::MatrixXf::Identity(numwavelengths, numwavelengths);
		
		RGB2Spec *model = rgb2spec_load("tables/srgb.coeff");

		set_rgb_spectrum(model);
		set_emit_spectrum(model);

		rgb2spec_free(model);
	};

	glm::vec3 get_material_color_from_spectrum(){
		glm::vec3 xyz = { 0.0, 0.0, 0.0 };
		for (int i = 0; i < numwavelengths; i++){
			xyz += daisy_color::cie1931WavelengthToXYZFit(wavelengths[i]) * spectral_values[i];
		}
		glm::vec3 rgb;
		daisy_color::XYZToRGB(xyz, rgb);
		return rgb;
	}

private:

	void set_rgb_spectrum(RGB2Spec *model){
		float coeff[3];
		float rgb[3] = { rgbcolor.x, rgbcolor.y, rgbcolor.z };
		// set spectrum from color rgb
		rgb2spec_fetch(model, rgb, coeff);
		for (int i = 0; i < numwavelengths; i++){
			float wavelength_from_rgb = rgb2spec_eval_precise(coeff, wavelengths[i]);
			spectral_values.push_back(wavelength_from_rgb);
		}
	}

	void set_emit_spectrum(RGB2Spec *model){
		float coeff[3];
		float emit[3] = { emission.x / 2.0, emission.y / 2.0, emission.z / 2.0 };
		// set spectrum from emission rgb
		rgb2spec_fetch(model, emit, coeff);
		for (int i = 0; i < numwavelengths; i++){
			float emission_from_rgb = rgb2spec_eval_precise(coeff, wavelengths[i]);
			spectral_emission.push_back(emission_from_rgb);
		}
	}

};
