#pragma once

class Lightning{
public:

	float emission_value;
	Eigen::VectorXf emission;
	Eigen::VectorXf residualvector;
	Eigen::VectorXf lightningvalues;

	int numpasses;
	SpMat RadMat;

	Lightning(MeshS &mesh, OptixPrimeFunctionality &optixP, float &emissionval){
		emission_value = emissionval;
		// initialize radiosity matrix
		RadMat = SpMat(mesh.numtriangles, mesh.numtriangles);
		optixP.calculateRadiosityMatrixStochastic(RadMat, mesh);

		setemission(mesh);
		// init lightning values
		lightningvalues = Eigen::VectorXf::Zero(mesh.numtriangles);
		lightningvalues = emission;
		// init residualvector
		residualvector = Eigen::VectorXf::Zero(mesh.numtriangles);
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