#pragma once

#include <glm/glm.hpp>
#include "rgb2spec.h"
#include "color.h"

class Material{
public:
	glm::vec3 rgbcolor; 
	glm::vec3 emission;

	int numwavelengths;

	glm::vec3 rgbspectralcolor;

	std::vector<float> spectral_values;
	std::vector<float> spectral_emission; 
	std::vector<float> &wavelengths;
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission, std::vector<float> &wave_lengths) :rgbcolor(rgbcolor), emission(emission), wavelengths(wave_lengths){
		numwavelengths = wave_lengths.size();
		// set spectrum from color rgb
		RGB2Spec *model = rgb2spec_load("tables/srgb.coeff");
		float rgb[3] = { rgbcolor.x, rgbcolor.y, rgbcolor.z }, coeff[3];
		rgb2spec_fetch(model, rgb, coeff);

		for (int i = 0; i < numwavelengths; i++){
			// set spectral value
			float wavelength_from_rgb = rgb2spec_eval_precise(coeff, wave_lengths[i]);
			spectral_values.push_back(wavelength_from_rgb);
			// set emission value
			float emission_from_rgb = rgb2spec_eval_precise(coeff, wave_lengths[i]);
			spectral_emission.push_back(emission_from_rgb);
		}

		// init rgbspectralcolor
		rgb2spec_fetch(model, rgb, coeff);
		float sample_600 = rgb2spec_eval_precise(coeff, 600);
		float sample_550 = rgb2spec_eval_precise(coeff, 550);
		float sample_450 = rgb2spec_eval_precise(coeff, 450);

		rgbspectralcolor = { sample_600, sample_550, sample_450 };

		// debug prints
		std::cout << "rgbspectralcolor, 600 = " << sample_600 << ", 550 = " << sample_550 << ", 450 = " << sample_450 << std::endl;
		std::cout << "now print from spectral_values: " << std::endl;
		for (int i = 0; i < numwavelengths; i++){
			std::cout << "wavelength is " << wave_lengths[i] << " spectral value is = " << spectral_values[i] << std::endl;
		}

		rgb2spec_free(model);
	};

	glm::vec3 get_material_color_from_rgbspectrum(){
		glm::vec3 xyz = daisy_color::cie1931WavelengthToXYZFit(600) * rgbspectralcolor[0]
						+ daisy_color::cie1931WavelengthToXYZFit(550) * rgbspectralcolor[1]
						+ daisy_color::cie1931WavelengthToXYZFit(450) * rgbspectralcolor[2];
		glm::vec3 rgb;
		daisy_color::XYZToRGB(xyz, rgb);
		return rgb;
	}


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
