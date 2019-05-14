#pragma once
#include <glm/glm.hpp>

class Camera{
public:
	optix::float3 debug_marijn_eye;
	optix::float3 debug_marijn_dir;
	optix::float3 eye;
	optix::float3 up;
	optix::float3 dir;
	glm::vec4 viewport;
	bool moving = false;
	int pixwidth, pixheight;

	void rotate(float yaw, float pitch, float roll){
		// let's rotate the camera
		optix::float3 new_eye = eye;
		optix::float3 new_up = up;

		// r is the vector from center to eye
		optix::float3 r_vec = new_eye - dir;
		optix::float3 rotated_r = yaw_pitch_eye(r_vec, yaw, pitch);
		new_eye = rotated_r + dir;

		// get new up vec
		optix::float3 n_vec = cross(r_vec, new_up);
		new_up = cross(n_vec, r_vec);
		new_up = normalize(new_up);

		// store our proud new vecs
		up = new_up;
		eye = new_eye;
	}

	optix::float3 yaw_pitch_eye(optix::float3 &eye_in, double yaw, double pitch){
		// this function actually first inputted yaw 0.0 pitch :s
		glm::mat4x4 rotation_mat = yaw_pitch_roll_in_degrees_to_mat(0.0, yaw, pitch);
		glm::vec4 homogen_eye = { eye_in.x, eye_in.y, eye_in.z, 1.0 };
		glm::vec4 rotated = rotation_mat * homogen_eye;
		optix::float3 res = {rotated.x, rotated.y, rotated.z};
		return res;	
	}

	// IN THIS FUNCTION SOMETHING IS WRONG, somehow swaps pitch and roll
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