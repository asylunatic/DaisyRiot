#include "Material.h"
#include "color.h"

Material::Material(glm::vec3 rgbcolor, glm::vec3 emission, glm::vec3 blacklightcolor, std::vector<float> &wave_lengths)
	:rgbcolor(rgbcolor), emission(emission), blacklightcolor(blacklightcolor), wavelengths(wave_lengths){

	numwavelengths = wave_lengths.size();
	M = Eigen::MatrixXf::Identity(numwavelengths, numwavelengths);

	RGB2Spec *model = rgb2spec_load("color_tables/srgb.coeff");

	rgb_to_spectrum(model, blacklightcolor, spectral_from_blacklight);
	set_fluorescent_matrix();

	rgb_to_spectrum(model, rgbcolor, spectral_values);
	rgb_to_spectrum(model, emission, spectral_emission);

	rgb2spec_free(model);
};

glm::vec3 Material::get_material_color_from_spectrum(){
	glm::vec3 xyz = { 0.0, 0.0, 0.0 };
	for (int i = 0; i < numwavelengths; i++){
		xyz += daisy_color::cie1931WavelengthToXYZFit(wavelengths[i]) * spectral_values[i];
	}
	glm::vec3 rgb;
	daisy_color::XYZToRGB(xyz, rgb);
	return rgb;
}

void Material::set_fluorescent_matrix(){
	float* ptr = &spectral_from_blacklight[0];
	Eigen::Map<Eigen::VectorXf> temp(ptr, numwavelengths);
	for (int i = 0; i < numwavelengths; i++){
		if (300.0 < wavelengths[i] < 400.0){
			M.col(i) = temp;
		}
	}
}

void Material::rgb_to_spectrum(RGB2Spec *model, glm::vec3 rgbcolor, std::vector<float> &spectrum){
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

UVLightMaterial::UVLightMaterial(glm::vec3 rgbcolor, glm::vec3 emission, glm::vec3 blacklightcolor, std::vector<float> &wave_lengths) :Material(rgbcolor, emission, blacklightcolor, wave_lengths){
	rgbcolor = { 0.0, 0.0, 0.0 };
	emission = { 0.0, 0.0, 0.0 };
	blacklightcolor = { 0.0, 0.0, 0.0 };
	resample_emission();
}

void UVLightMaterial::resample_emission(){

}