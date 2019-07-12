#pragma once
#include "ImageExporter.h"
#include <iostream>
#include <fstream>
#include "color.h"

class Lightning{
public:
	static Lightning* get_lightning(int method, MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval, std::vector<float> wavelengthsvec,
		bool cuda_enabled = false, char* matfile = nullptr);
	virtual glm::vec3 get_color_of_patch(int) = 0; 
	virtual void converge_lightning() = 0;
	virtual void increment_lightpass() = 0;
	virtual void reset() = 0;

protected:
	bool cuda_on = false;
	int numpasses;
	SpMat RadMat;
	float emission_value;
	void SerializeMat(SpMat& m, char* matfile) {
		std::vector<Tripl> res;
		int sz = m.nonZeros();
		m.makeCompressed();

		std::fstream writeFile;
		writeFile.open(matfile, std::ios::binary | std::ios::out);

		if (writeFile.is_open())
		{
			int rows, cols, nnzs, outS, innS;
			rows = m.rows();
			cols = m.cols();
			nnzs = m.nonZeros();
			outS = m.outerSize();
			innS = m.innerSize();

			writeFile.write((const char *)&(rows), sizeof(int));
			writeFile.write((const char *)&(cols), sizeof(int));
			writeFile.write((const char *)&(nnzs), sizeof(int));
			writeFile.write((const char *)&(outS), sizeof(int));
			writeFile.write((const char *)&(innS), sizeof(int));

			writeFile.write((const char *)(m.valuePtr()), sizeof(float) * m.nonZeros());
			writeFile.write((const char *)(m.outerIndexPtr()), sizeof(int) * m.outerSize());
			writeFile.write((const char *)(m.innerIndexPtr()), sizeof(int) * m.nonZeros());

			writeFile.close();
		}
	}
	void DeserializeMat(SpMat& m, char* matfile) {
		std::fstream readFile;
		readFile.open(matfile, std::ios::binary | std::ios::in);
		if (readFile.is_open())
		{
			int rows, cols, nnz, inSz, outSz;
			readFile.read((char*)&rows, sizeof(int));
			readFile.read((char*)&cols, sizeof(int));
			readFile.read((char*)&nnz, sizeof(int));
			readFile.read((char*)&inSz, sizeof(int));
			readFile.read((char*)&outSz, sizeof(int));

			m.resize(rows, cols);
			m.makeCompressed();
			m.resizeNonZeros(nnz);

			readFile.read((char*)(m.valuePtr()), sizeof(float) * nnz);
			readFile.read((char*)(m.outerIndexPtr()), sizeof(int) * outSz);
			readFile.read((char*)(m.innerIndexPtr()), sizeof(int) * nnz);

			m.finalize();
			readFile.close();
		}
	}
	void initMat(MeshS &mesh, OptixPrimeFunctionality &optixP){
		RadMat = SpMat(mesh.numtriangles, mesh.numtriangles);
		if (cuda_on){
			optixP.cudaCalculateRadiosityMatrix(RadMat, mesh);
		}
		else{
			optixP.calculateRadiosityMatrix(RadMat, mesh);
		}
	}
	void initMatFromFile( MeshS &mesh, OptixPrimeFunctionality &optixP, char* matfile){
		RadMat = SpMat(mesh.numtriangles, mesh.numtriangles);
		bool fileExists = ImageExporter::exists(matfile);
		if (fileExists){
			DeserializeMat(RadMat, matfile);
			std::cout << "Deserialized matrix" << std::endl;
		}
		else{
			initMat(mesh, optixP);
			SerializeMat(RadMat, matfile);
			std::cout << "Loaded & Serialized matrix" << std::endl;
		}
	}
};

class SpectralLightning : public Lightning{
private:
	int numsamples;
	int numpatches;
	std::vector<glm::vec3> xyz_per_wavelength;
	std::vector<glm::vec3> rgb_color_cache;		// here we cache the rgb colors of the patches
	MeshS& mesh_;

	std::vector<Eigen::VectorXf> emission;
	std::vector<Eigen::VectorXf> residualvector;
	std::vector<Eigen::VectorXf> lightningvalues; // per color channel
	std::vector<Eigen::VectorXf> reflectionvalues; // how much of which color is reflected per patch
	std::vector<Eigen::MatrixXf> reflectionmatrix; // how much of a wavelength is reflected to which other wavelengths, per patch

public:
	SpectralLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval, std::vector<float> wavelengthsvec, bool cuda_enabled = false, char* matfile = nullptr):mesh_(mesh){
		cuda_on = cuda_enabled;
		numsamples = wavelengthsvec.size();
		numpatches = mesh.numtriangles;
		xyz_per_wavelength = get_xyz_conversions(wavelengthsvec);
		emission_value = emissionval;
		set_sampled_emission(mesh);
		/*std::cout << "sampled emission" << std::endl;
		for (int i = 0; i < numsamples; i++){
			std::cout << emission[i] << std::endl;
		}*/
		set_reflectionvalues(mesh);
		set_reflectionmatrix(mesh);
		if (matfile == nullptr){
			initMat(mesh, optixP);
		}
		else{
			initMatFromFile(mesh, optixP, matfile);
		}
		//std::cout << "Radmat " << std::endl;
		//std::cout << Eigen::MatrixXf(RadMat) << std::endl;
		reset();
		std::cout << "Lightning has been initialized" << std::endl;
		//converge_lightning();
		//std::cout << "The lightning is converged" << std::endl;
	}

	glm::vec3 get_color_of_patch(int index){
		return rgb_color_cache[index];
	}

	void converge_lightning(){
		while (check_convergence(residualvector) > 50){
			std::cout << "Residual light in scene: " << check_convergence(residualvector) << std::endl;
			increment_light_fluorescent();
		}
		update_color_cache();
	}

	void increment_lightpass(){
		increment_light_fluorescent();
		update_color_cache();
		numpasses++;
	}

	void reset(){
		rgb_color_cache = std::vector<glm::vec3>(numpatches, { 0.0, 0.0, 0.0 });
		residualvector = emission;
		lightningvalues = emission;
		update_color_cache();
		numpasses = 0;
	}

private:
	void update_color_cache(){
		// update color per patch
		for (int i = 0; i < rgb_color_cache.size(); i++){
			glm::vec3 xyz;
			for (int j = 0; j < numsamples; j++){
				xyz += xyz_per_wavelength[j] * lightningvalues[j][i];
			}
			glm::vec3 rgb = { 0.0, 0.0, 0.0 };
			daisy_color::XYZToRGB(xyz, rgb);
			float maxval = std::fmaxf(rgb[0], std::fmaxf(rgb[1], rgb[2]));
			if (maxval > 1){
				rgb = { rgb[0] / maxval, rgb[1] / maxval, rgb[2] / maxval };
			}
			rgb_color_cache[i] = rgb;
		}
	}

	//void increment_light_fast(){
	//	for (int i = 0; i < numsamples; i++){
	//		residualvector[i] = (RadMat * residualvector[i]).cwiseProduct(reflectionvalues[i]);
	//		//mult_material_matrix(residualvector);
	//		lightningvalues[i] = lightningvalues[i] + residualvector[i];
	//		mult_material_matrix(lightningvalues);
	//	}
	//	numpasses++;
	//}

	// none of this horrible function would have been necesarry had we used a sensible library for matrix/vector stuff that supports broadcasting
	void increment_light_fluorescent(){
		// make mat for bounced light
		std::vector<Eigen::VectorXf> bounced_light(numsamples);
		// first bounce all the light
		for (int i = 0; i < numsamples; i++){
			bounced_light[i] = (RadMat * residualvector[i]);
		}

		// now calculate per patch, per wavelength to which wavelengths the light is reflected
		for (int i = 0; i < mesh_.numtriangles; i++){
			// first, get row
			Eigen::VectorXf patchrowvec = Eigen::VectorXf(numsamples);
			for (int j = 0; j < numsamples; j++){
				patchrowvec[j] = bounced_light[j][i];
			}
			// now multiply the row vector with the material matrix of the patch
			Eigen::VectorXf result = reflectionmatrix[i] * patchrowvec;

			// store the result in residual light matrix
			for (int j = 0; j < numsamples; j++){
				residualvector[j][i] = result[j];
			}
		}

		// add to lightning values
		for (int i = 0; i < numsamples; i++){
			lightningvalues[i] = lightningvalues[i] + residualvector[i];
		}

		numpasses++;
	}

	void mult_material_matrix(std::vector<Eigen::VectorXf> &vectorlist){
		int numtriangles_ = mesh_.numtriangles;
		for (int i = 0; i < numtriangles_; i++){
			// make vector from row
			Eigen::VectorXf patchrowvec = Eigen::VectorXf(numsamples);
			for (int j = 0; j < numsamples; j++){
				patchrowvec[j] = vectorlist[j][i];
			}
			// mult rowvec with matrix
			Eigen::VectorXf result = reflectionmatrix[i] * patchrowvec;
			// put result back in lightningvalues
			for (int j = 0; j < numsamples; j++){
				vectorlist[j][i] = result[j];
			}
		}
	}

	std::vector<glm::vec3> get_xyz_conversions(std::vector<float> wavelengthsvec){
		std::vector<glm::vec3> res;
		res.resize(numsamples);
		for (int i = 0; i < numsamples; i++){
			glm::vec3 xyz = daisy_color::cie1931WavelengthToXYZFit(wavelengthsvec[i]);
			res[i] = xyz;
		}
		return res;
	}

	float check_convergence(std::vector<Eigen::VectorXf> input){
		float sum = 0.0;
		for (int i = 0; i < numsamples; i++){
			sum += input[i].sum();
		}
		return sum;
	}

	void set_sampled_emission(MeshS &mesh){
		emission.resize(numsamples);
		for (int i = 0; i < numsamples; i++){
			emission[i] = Eigen::VectorXf::Zero(mesh.numtriangles);
			for (int j = 0; j < mesh.numtriangles; j++){
				if (mesh.materials[mesh.materialIndexPerTriangle[j]].spectral_emission[i] > 0.0){
					emission[i][j] = mesh.materials[mesh.materialIndexPerTriangle[j]].spectral_emission[i] * emission_value;
				}
			}
		}
	}

	void set_reflectionvalues(MeshS &mesh){
		reflectionvalues.resize(numsamples);
		for (int i = 0; i < numsamples; i++){
			reflectionvalues[i] = Eigen::VectorXf::Zero(mesh.numtriangles);
			for (int j = 0; j < mesh.numtriangles; j++){
				if (mesh.materials[mesh.materialIndexPerTriangle[j]].spectral_values[i] > 0.0){
					reflectionvalues[i](j) = mesh.materials[mesh.materialIndexPerTriangle[j]].spectral_values[i];
				}
			}
		}
	}

	void set_reflectionmatrix(MeshS &mesh){
		reflectionmatrix.resize(mesh.numtriangles);
		for (int i = 0; i < mesh.numtriangles; i++){
			reflectionmatrix[i] = mesh_.materials[mesh_.materialIndexPerTriangle[i]].M;
		}
	}


};


class RGBLightning : public Lightning{
private:
	float emission_value;
	int numsamples = 3;
	std::vector<Eigen::VectorXf> emission;
	std::vector<Eigen::VectorXf> residualvector;
	std::vector<Eigen::VectorXf> lightningvalues; // per color channel
	std::vector<Eigen::VectorXf> reflectionvalues; // how much of which color is reflected per patch

public:
	RGBLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval){
		emission_value = emissionval;
		set_sampled_emission(mesh);
		set_reflectionvalues(mesh);
		initMat(mesh, optixP);
		reset();
		converge_lightning();
	}

	RGBLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval, bool cuda_enabled = false, char* matfile = nullptr){
		cuda_on = cuda_enabled;
		emission_value = emissionval;
		set_sampled_emission(mesh);
		set_reflectionvalues(mesh);
		if (matfile == nullptr){
			initMat(mesh, optixP);
		}
		else{
			initMatFromFile(mesh, optixP, matfile);
		}
		reset();
		converge_lightning();
	}

	glm::vec3 get_color_of_patch(int index){
		return{ lightningvalues[0][index], lightningvalues[1][index], lightningvalues[2][index] };
	}

	void converge_lightning(){
		while (residualvector[0].sum() > 0.0001 || residualvector[1].sum() > 0.0001 || residualvector[2].sum() > 0.0001){
			increment_lightpass();
		}
	}

	void increment_lightpass(){
		for (int i = 0; i < numsamples; i++){
			residualvector[i] = (RadMat * residualvector[i]).cwiseProduct(reflectionvalues[i]);
			lightningvalues[i] = lightningvalues[i] + residualvector[i];
		}
		// multiply with matrix first
		numpasses++;
	}

	void reset(){
		residualvector = emission;
		lightningvalues = emission;
		numpasses = 0;
	}

private:

	void set_sampled_emission(MeshS &mesh){
		emission.resize(numsamples);
		for (int i = 0; i < numsamples; i++){
			emission[i] = Eigen::VectorXf::Zero(mesh.numtriangles);
			// set emissive values from material
			for (int j = 0; j < mesh.numtriangles; j++){
				if (mesh.materials[mesh.materialIndexPerTriangle[j]].emission[i] > 0.0){
					emission[i][j] = mesh.materials[mesh.materialIndexPerTriangle[j]].emission[i] * emission_value;
				}
			}
		}
	}

	void set_reflectionvalues(MeshS &mesh){
		reflectionvalues.resize(numsamples);
		for (int i = 0; i < numsamples; i++){
			reflectionvalues[i] = Eigen::VectorXf::Zero(mesh.numtriangles);
			for (int j = 0; j < mesh.numtriangles; j++){
				if (mesh.materials[mesh.materialIndexPerTriangle[j]].rgbcolor[i] > 0.0){
					reflectionvalues[i](j) = mesh.materials[mesh.materialIndexPerTriangle[j]].rgbcolor[i];
				}
			}
		}
	}

};

class BWLightning: public Lightning{
public:

	Eigen::VectorXf emission;
	Eigen::VectorXf residualvector;
	Eigen::VectorXf lightningvalues;
	
	BWLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval, char* matfile = nullptr){
		emission_value = emissionval;
		setemission(mesh);
		if (matfile == nullptr){
			initMat(mesh, optixP);
		}
		else{
			initMatFromFile(mesh, optixP, matfile);
		}
		reset();
		converge_lightning();
	}

	glm::vec3 get_color_of_patch(int index){
		return{ lightningvalues[index], lightningvalues[index], lightningvalues[index] };
	}

	void converge_lightning(){
		while (residualvector.sum() > 0.0001){
			residualvector = RadMat * residualvector;
			lightningvalues = lightningvalues + residualvector;
			numpasses++;
		}
		std::cout << "Number of light passes " << numpasses << ". Amount of residual light in scene " << residualvector.sum() << std::endl;
	}

	void increment_lightpass(){
		residualvector = RadMat * residualvector;
		lightningvalues = lightningvalues + residualvector;
		numpasses++;
		std::cout << "Number of light passes " << numpasses << ". Amount of residual light in scene " << residualvector.sum() << std::endl;
	}

	void reset(){
		residualvector = emission;
		lightningvalues = emission;
		numpasses = 0;
	}

private:

	void setemission(MeshS &mesh){
		emission = Eigen::VectorXf::Zero(mesh.numtriangles);
		// set emissive values from material
		for (int i = 0; i < mesh.numtriangles; i++){
			if (mesh.materials[mesh.materialIndexPerTriangle[i]].emission[0] > 0.0){
				emission(i) = mesh.materials[mesh.materialIndexPerTriangle[i]].emission[0] * emission_value;
			}
		}
	}
};


inline Lightning* Lightning::get_lightning(int method, MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval, std::vector<float> wavelengthsvec,
	bool cuda_enabled , char* matfile){
	if (method == 0){
		return new BWLightning(mesh, optixP, emissionval, matfile);
	}
	else if (method == 1){
		return new RGBLightning(mesh, optixP, emissionval, cuda_enabled, matfile);
	}
	else if (method == 2){
		return new SpectralLightning(mesh, optixP, emissionval, wavelengthsvec, cuda_enabled, matfile);
	}
}