#pragma once

#include <vector>
#include "rgb2spec.h"
#include <glm/glm.hpp>
#include <Eigen\Dense>

class Material{
public:
	glm::vec3 rgbcolor;
	glm::vec3 emission;

	std::vector<float> spectral_values;
	std::vector<float> spectral_emission; 
	std::vector<float> &wavelengths;

	Eigen::MatrixXf M; // matrix to influence wavelengths
	
	Material(glm::vec3 rgbcolor, glm::vec3 emission, std::vector<float> &wave_lengths);
	glm::vec3 get_material_color_from_spectrum();

protected:
	int numwavelengths;
	void rgb_to_spectrum(RGB2Spec *model, glm::vec3 rgbcolor, std::vector<float> &spectrum);
};

class UVLightMaterial : public Material{
public: 
	UVLightMaterial(glm::vec3 rgbcolor, glm::vec3 emission, std::vector<float> &wave_lengths);
private:
	void resample_emission(std::vector<float> &wave_lengths);
	float sample_bell_curve(float x);
};

class FluorescentMaterial : public Material{
public:
	FluorescentMaterial(glm::vec3 rgbcolor, glm::vec3 emission, glm::vec3 blacklightcolor, std::vector<float> &wave_lengths);
	glm::vec3 blacklightcolor;
	std::vector<float> spectral_from_blacklight;

private:
	void set_fluorescent_matrix();
};