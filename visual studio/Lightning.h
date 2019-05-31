#pragma once

class Lightning{
public:

	float emission_value;
	Eigen::VectorXf emission;
	int numsamples = 3;
	std::vector<Eigen::VectorXf> emissionSampled;
	Eigen::VectorXf residualvector;
	Eigen::VectorXf lightningvalues;

	int numpasses;
	SpMat RadMat;

	Lightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval){
		emission_value = emissionval;

		setemission(mesh);
		setSampledEmission(mesh);

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

	void converge_lightning(){
		while (residualvector.sum() > 0.0001){
			residualvector = RadMat * residualvector;
			lightningvalues = lightningvalues + residualvector;
			numpasses++;
		}
	}

	void increment_lightpass(){
		residualvector = RadMat * residualvector;
		lightningvalues = lightningvalues + residualvector;
		numpasses++;
	}

	void reset(){
		residualvector = emission;
		lightningvalues = emission;
		numpasses = 0;
	}

private:
	void setSampledEmission(MeshS &mesh){
		emissionSampled.resize(numsamples);
		for (int i = 0; i < numsamples; i++){
			emissionSampled[i] = Eigen::VectorXf::Zero(mesh.numtriangles);
			// set emissive values from material
			for (int j = 0; j < mesh.numtriangles; j++){
				if (mesh.materials[mesh.materialIndexPerTriangle[j]].emission[i] > 0.0){
					emissionSampled[i](j) = mesh.materials[mesh.materialIndexPerTriangle[j]].emission[i] * emission_value;
				}
			}
		}
	}
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