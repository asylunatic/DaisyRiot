#pragma once

#include <glm/glm.hpp>
#include "rgb2spec.h"
#include "color.h"

class Material{
public:
	glm::vec3 rgbcolor; 
	glm::vec3 emission;

	int numwavelengths;

	std::vector<float> spectral_values;
	std::vector<float> spectral_emission; 
	std::vector<float> &wavelengths;
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission, std::vector<float> &wave_lengths) :rgbcolor(rgbcolor), emission(emission), wavelengths(wave_lengths){
		numwavelengths = wave_lengths.size();

		RGB2Spec *model = rgb2spec_load("tables/srgb.coeff");

		// set spectrum from color rgb
		float rgb[3] = { rgbcolor.x, rgbcolor.y, rgbcolor.z }, coeff[3];
		rgb2spec_fetch(model, rgb, coeff);
		for (int i = 0; i < numwavelengths; i++){
			float wavelength_from_rgb = rgb2spec_eval_precise(coeff, wave_lengths[i]);
			spectral_values.push_back(wavelength_from_rgb);
		}


		// set spectrum from emission rgb
		float emit[3] = { emission.x/2.0, emission.y/2.0, emission.z/2.0 };
		rgb2spec_fetch(model, emit, coeff);
		for (int i = 0; i < numwavelengths; i++){
			// set emission value
			float emission_from_rgb = rgb2spec_eval_precise(coeff, wave_lengths[i]);
			spectral_emission.push_back(emission_from_rgb);
		}


		for (int i = 0; i < numwavelengths; i++){
			std::cout << "wavelength is " << wave_lengths[i] << " spectral value is = " << spectral_values[i] << std::endl;
		}

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

	glm::vec3 get_color_from_spectrum(std::vector<float> &spectral_values_vec){
		glm::vec3 xyz = { 0.0, 0.0, 0.0 };
		for (int i = 0; i < numwavelengths; i++){
			xyz += daisy_color::cie1931WavelengthToXYZFit(wavelengths[i]) * spectral_values_vec[i];
		}
		glm::vec3 rgb;
		daisy_color::XYZToRGB(xyz, rgb);
		return rgb;
	}

};
