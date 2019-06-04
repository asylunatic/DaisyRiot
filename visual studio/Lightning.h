#pragma once

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
};

class RGBLightning : public Lightning{
public:
	float emission_value;
	std::vector<Eigen::VectorXf> emission;
	int numsamples = 3;
	std::vector<Eigen::VectorXf> residualvector;
	std::vector<Eigen::VectorXf> lightningvalues; // per color channel
	std::vector<Eigen::VectorXf> reflectionvalues; // how much of which color is reflected per patch

	int numpasses;
	SpMat RadMat;

	RGBLightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval){
		emission_value = emissionval;
		set_sampled_emission(mesh);
		set_reflectionvalues(mesh);

		// initialize radiosity matrix
		RadMat = SpMat(mesh.numtriangles, mesh.numtriangles);
		optixP.calculateRadiosityMatrixStochastic(RadMat, mesh);

		lightningvalues = emission;
		residualvector = emission;
		// first lightning
		numpasses = 0;

		// converge lightning
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
			residualvector[i] = (RadMat * residualvector[i]) * reflectionvalues[i];
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
					emission[i](j) = mesh.materials[mesh.materialIndexPerTriangle[j]].emission[i] * emission_value;
				}
			}
		}
	}

	void set_reflectionvalues(MeshS &mesh){
		reflectionvalues.resize(numsamples);
		for (int i = 0; i < numsamples; i++){
			reflectionvalues[i] = Eigen::VectorXf::Zero(mesh.numtriangles);
			for (int j = 0; j < mesh.numtriangles; j++){
				if (mesh.materials[mesh.materialIndexPerTriangle[j]].rgbcolor[2-i] > 0.0){
					reflectionvalues[i](j) = mesh.materials[mesh.materialIndexPerTriangle[j]].rgbcolor[2-i];
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

		// initialize radiosity matrix
		RadMat = SpMat(mesh.numtriangles, mesh.numtriangles);
		optixP.calculateRadiosityMatrixStochastic(RadMat, mesh);

		lightningvalues = emission;
		residualvector = emission;
		// first lightning
		numpasses = 0;

		// converge lightning
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