#pragma once
#include "ImageExporter.h"
#include <iostream>
#include <fstream>

class Lightning{
public:
	virtual glm::vec3 get_color_of_patch(int) = 0; 
	virtual void converge_lightning() = 0;
	virtual void increment_lightpass() = 0;
	virtual void reset() = 0;

protected:
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
		optixP.calculateRadiosityMatrix(RadMat, mesh); 
	}
	void initMatFromFile( MeshS &mesh, OptixPrimeFunctionality &optixP, char* matfile){
		RadMat = SpMat(mesh.numtriangles, mesh.numtriangles);
		bool fileExists = ImageExporter::exists(matfile);
		if (fileExists){
			DeserializeMat(RadMat, matfile);
			std::cout << "Deserialized matrix" << std::endl;
		}
		else{
			optixP.calculateRadiosityMatrix(RadMat, mesh);
			SerializeMat(RadMat, matfile);
			std::cout << "Loaded & Serialized matrix" << std::endl;
		}
	}
};

class SpectralLightning : public Lightning{
private:
	int numsamples;
	std::vector<glm::vec3> xyz_per_wavelength;
	Eigen::MatrixXf emissionmat;
	Eigen::MatrixXf residualmat;
	Eigen::MatrixXf lightningmat;
	Eigen::MatrixXf reflectionmat;

public:
	SpectralLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval, std::vector<float> wavelengthsvec, char* matfile = nullptr){
		xyz_per_wavelength = get_xyz_conversions(wavelengthsvec);
		numsamples = wavelengthsvec.size();
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
		// get row from lightningvales
		Eigen::VectorXf spectral_values_vec_from_mat = lightningmat.row(index);

		// pairwise multiplication to get color
		glm::vec3 xyzcolor = { 0.0, 0.0, 0.0 };
		for (int i = 0; i < numsamples; i++){
			xyzcolor += xyz_per_wavelength[i] * spectral_values_vec_from_mat[i];
		}
		glm::vec3 rgbcolor;
		daisy_color::XYZToRGB(xyzcolor, rgbcolor);
		return rgbcolor;
	}

	void converge_lightning(){
		while (check_convergence(residualmat) > 0.0001){
			increment_lightpass();
		}
	}

	void increment_lightpass(){
		for (int i = 0; i < numsamples; i++){
			residualmat.col(i) = (RadMat * residualmat.col(i)).cwiseProduct(reflectionmat.col(i));
		}
		lightningmat = lightningmat + residualmat;

		numpasses++;
	}

	void reset(){
		residualmat = emissionmat;
		lightningmat = emissionmat;
		numpasses = 0;
	}

private:
	std::vector<glm::vec3> get_xyz_conversions(std::vector<float> wavelengthsvec){
		std::vector<glm::vec3> res = {};
		for (int i = 0; i < wavelengthsvec.size(); i++){
			glm::vec3 xyz = daisy_color::cie1931WavelengthToXYZFit(wavelengthsvec[i]);
			res.push_back(xyz);
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

	float check_convergence(Eigen::MatrixXf input){
		float sum = input.sum();
		return sum;
	}

	void set_sampled_emission(MeshS &mesh){
		// MAT
		emissionmat = Eigen::MatrixXf::Zero(mesh.numtriangles, numsamples);
		for (int i = 0; i < numsamples; i++){
			// set emissive values from material
			for (int j = 0; j < mesh.numtriangles; j++){
				if (mesh.materials[mesh.materialIndexPerTriangle[j]].spectral_emission[i] > 0.0){
					emissionmat(j, i) = mesh.materials[mesh.materialIndexPerTriangle[j]].spectral_emission[i] * emission_value;
				}
			}
		}
	}

	void set_reflectionvalues(MeshS &mesh){
		// MAT
		reflectionmat = Eigen::MatrixXf::Zero(mesh.numtriangles, numsamples);
		for (int i = 0; i < numsamples; i++){
			for (int j = 0; j < mesh.numtriangles; j++){
				reflectionmat.col(i)[j] = mesh.materials[mesh.materialIndexPerTriangle[j]].spectral_values[i];
			}
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

	RGBLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval, char* matfile){
		emission_value = emissionval;
		set_sampled_emission(mesh);
		set_reflectionvalues(mesh);
		initMatFromFile(mesh, optixP, matfile);
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
	
	BWLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval){
		emission_value = emissionval;
		setemission(mesh);
		initMat(mesh, optixP);
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