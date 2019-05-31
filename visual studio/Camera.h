#pragma once
#include <glm/glm.hpp>

class Camera{
public:
	float totalpitch;
	const float max_pitch = 45.f;
	optix::float3 eye;
	optix::float3 up;
	optix::float3 dir;
	glm::vec4 viewport;
	bool moving = false;
	int pixwidth, pixheight;

	Camera(int width, int height){
		eye = optix::make_float3(0.0f, 0.0f, 10.0f);
		dir = optix::make_float3(0.0f, 0.0f, 0.0f);
		up = optix::make_float3(0.0f, 1.0f, 0.0f);
		totalpitch = 0.0;
		viewport = { 0.0f, 0.0f, float(width), float(height) };
		pixwidth = width;
		pixheight = height;
	}

	void rotate(float yaw, float pitch, float roll){

		if ( max_pitch - abs(totalpitch + pitch) < 0){ 
			std::cout << "preventing gimbal lock, totalpitch is = "<< totalpitch << " want to add " << pitch << " pitch " << std::endl;
			pitch = 0;
		}

		// let's rotate the camera
		// r is the vector from center to eye
		optix::float3 r_vec = eye - dir;
		optix::float3 rotated_r = yaw_pitch_eye(r_vec, yaw, pitch);
		optix::float3 new_eye = rotated_r + dir;

		// get new up 
		optix::float3 new_up = up;
		optix::float3 uppie = optix::make_float3(0.0f, 1.0f, 0.0f);
		//optix::float3 n_vec = cross(r_vec, uppie); //0.0, 1.0, 0.0
		//new_up = cross(n_vec, r_vec);
		new_up = normalize(new_up);

		// store our proud new vecs
		up = new_up;
		eye = new_eye;

		totalpitch += pitch;
	}
	void gen_rays_for_screen(std::vector<optix::float3> &rays){
		rays.resize(pixwidth*pixheight * 2);

		// generate rays the un_project way
		glm::mat4x4 lookat = glm::lookAt(optix_functionality::optix2glmf3(eye), optix_functionality::optix2glmf3(dir), optix_functionality::optix2glmf3(up));
		glm::mat4x4 projection = glm::perspective(45.0f, (float)(800) / (float)(600), 0.1f, 1000.0f);
		for (size_t x = 0; x < pixwidth; x++) {
			for (size_t y = 0; y < pixheight; y++) {
				// get ray origin
				glm::vec3 win(x, y, 0.0);
				glm::vec3 world_coord = glm::unProject(win, lookat, projection, viewport);
				rays[(y*pixwidth + x) * 2] = optix_functionality::glm2optixf3(world_coord);
				// get ray direction
				glm::vec3 win_dir(x, y, 1.0);
				glm::vec3 dir_coord = glm::unProject(win_dir, lookat, projection, viewport);
				rays[((y*pixwidth + x) * 2) + 1] = optix_functionality::glm2optixf3(dir_coord);
			}
		}
	}

private:
	optix::float3 yaw_pitch_eye(optix::float3 &eye_in, double yaw, double pitch){
		// this function actually first inputted yaw 0.0 pitch 
		glm::mat4x4 rotation_mat = yaw_pitch_roll_in_degrees_to_mat(yaw, 0.0, pitch);
		//glm::mat4x4 rot_mat = glm::yawPitchRoll(yaw, pitch, 0.0);
		glm::vec4 homogen_eye = { eye_in.x, eye_in.y, eye_in.z, 1.0 };
		glm::vec4 rotated = rotation_mat * homogen_eye;
		optix::float3 res = {rotated.x, rotated.y, rotated.z};
		return res;	

	}

	glm::mat4x4 yaw_pitch_roll_in_degrees_to_mat(double yaw, double pitch, double roll){
		//derived from http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/ but on a side note, it does not comply with the code stated there
		yaw = to_radians(yaw);
		pitch = to_radians(pitch);
		roll = to_radians(roll);
		double c1 = cos(yaw / 2.0);
		double c2 = cos(pitch / 2.0);
		double c3 = cos(roll / 2.0);
		double s1 = sin(yaw / 2.0);
		double s2 = sin(pitch / 2.0);
		double s3 = sin(roll / 2.0);

		double q0 = s1 * s2 * c3 + c1 * c2 * s3;
		double q1 = s1 * c2 * c3 + c1 * s2 * s3;
		double q2 = c1 * s2 * c3 - s1 * c2 * s3;
		double q3 = c1 * c2 * c3 - s1 * s2 * s3;

		// Copied from http://www.gamasutra.com/features/19980703/quaternions_01.htm
		double wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

		// calculate coefficients
		x2 = q0 + q0;
		y2 = q1 + q1;
		z2 = q2 + q2;

		xx = q0 * x2;
		xy = q0 * y2;
		xz = q0 * z2;

		yy = q1 * y2;
		yz = q1 * z2;

		zz = q2 * z2;

		wx = q3 * x2;
		wy = q3 * y2;
		wz = q3 * z2;

		float m0 = 1.0 - (yy + zz);
		float m1 = xy - wz;
		float m2 = xz + wy;

		float m3 = xy + wz;
		float m4 = 1.0 - (xx + zz);
		float m5 = yz - wx;

		float m6 = xz - wy;
		float m7 = yz + wx;
		float m8 = 1.0 - (xx + yy);

		glm::mat4x4 m(m0, m1, m2, 0.0f,
					m3, m4, m5, 0.0f,
					m6, m7, m8, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
		return m;
	}

	double to_radians(double in){
		return (in / 180.0)*M_PIf;
	}

};