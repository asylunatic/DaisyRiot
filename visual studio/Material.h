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
	std::vector<float> spectral_from_blacklight;
	std::vector<float> &wavelengths;

	Eigen::MatrixXf M; // matrix to influence wavelengths
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission, glm::vec3 blacklight_2_rgb, std::vector<float> &wave_lengths)
		:rgbcolor(rgbcolor), emission(emission), blacklight_2_rgb(blacklight_2_rgb), wavelengths(wave_lengths){

		numwavelengths = wave_lengths.size();
		M = Eigen::MatrixXf::Identity(numwavelengths, numwavelengths);
		
		RGB2Spec *model = rgb2spec_load("tables/srgb.coeff");

		set_rgb_to_spectrumvec(model, blacklight_2_rgb, spectral_from_blacklight);
		set_fluorescent_matrix();

		set_rgb_to_spectrumvec(model, rgbcolor, spectral_values);
		set_rgb_to_spectrumvec(model, emission, spectral_emission);

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

	void set_fluorescent_matrix(){
		float* ptr = &spectral_from_blacklight[0];
		Eigen::Map<Eigen::VectorXf> temp(ptr, numwavelengths);
		for (int i = 0; i < numwavelengths; i++){
			if (300.0 < wavelengths[i] < 400.0){
				M.col(i) = temp;
			}
		}
	}

	void set_rgb_to_spectrumvec(RGB2Spec *model, glm::vec3 rgbcolor, std::vector<float> &spectrum){
		float coeff[3];
		float rgb[3] = { rgbcolor.x, rgbcolor.y, rgbcolor.z };
		// set spectrum from color rgb
		spectrum = {};
		rgb2spec_fetch(model, rgb, coeff);
		for (int i = 0; i < numwavelengths; i++){
			float wavelength_from_rgb = rgb2spec_eval_precise(coeff, wavelengths[i]);
			spectrum.push_back(wavelength_from_rgb);
		}
	}

};
