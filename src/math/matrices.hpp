#pragma once

#include "mat4.hpp"
#include "quaternion.hpp"



namespace math
{
	inline mat4 Translation(float x, float y, float z)
	{
		mat4 result(1);

		result.m03 = x;
		result.m13 = y;
		result.m23 = z;
		return result;
	}

	inline mat4 Translation(const vec3& pos)
	{
		mat4 result(1);

		result.m03 = pos.x;
		result.m13 = pos.y;
		result.m23 = pos.z;
		return result;
	}
		
	inline mat4 Rotation(float x, float y, float z)
	{
		mat4 mx(1.0f);
		mat4 my(1.0f);
		mat4 mz(1.0f);
		float c_mx(std::cosf(x * 0.0174533f)), s_mx(std::sinf(x * 0.0174533f));
		float c_my(std::cosf(y * 0.0174533f)), s_my(std::sinf(y * 0.0174533f));
		float c_mz(std::cosf(z * 0.0174533f)), s_mz(std::sinf(z * 0.0174533f));

		mx.m11 = c_mx;
		mx.m22 = c_mx;
		mx.m12 = -s_mx;
		mx.m21 = s_mx;

		my.m00 = c_my;
		my.m22 = c_my;
		my.m20 = -s_my;
		my.m02 = s_my;

		mz.m00 = c_mz;
		mz.m11 = c_mz;
		mz.m01 = -s_mz;
		mz.m10 = s_mz;

		return mz * (my * mx);
	}

	inline mat4 Rotation(const quaternion& q)
	{
		mat4 ret(1);

		float xx = q.x * q.x;
		float xy = q.x * q.y;
		float xz = q.x * q.z;
		float xw = q.x * q.w;

		float yy = q.y * q.y;
		float yz = q.y * q.z;
		float yw = q.y * q.w;

		float zz = q.z * q.z;
		float zw = q.z * q.w;

		ret.m00 = 1 - 2 * (yy + zz);
		ret.m01 = 2 * (xy - zw);
		ret.m02 = 2 * (xz + yw);

		ret.m10 = 2 * (xy + zw);
		ret.m11 = 1 - 2 * (xx + zz);
		ret.m12 = 2 * (yz - xw);

		ret.m20 = 2 * (xz - yw);
		ret.m21 = 2 * (yz + xw);
		ret.m22 = 1 - 2 * (xx + yy);
		return ret;
	}

	inline mat4 Scaling(float x, float y, float z)
	{
		mat4 result(1);

		result.m00 = x;
		result.m11 = y;
		result.m22 = z;
		return result;
	}

	inline mat4 Scaling(const vec3& scl)
	{
		mat4 result(1);

		result.m00 = scl.x;
		result.m11 = scl.y;
		result.m22 = scl.z;
		return result;
	}

	inline mat4 CameraMatrix(vec3 target, vec3 up)
	{
		target.normalize();
		up.normalize();
		up = up.cross(target);
		vec3 v = target.cross(up);

		mat4 m(1);

		m.m00 = up[0];		m.m01 = up[1];		m.m02 = up[2];
		m.m10 = v[0];		m.m11 = v[1];		m.m12 = v[2];
		m.m20 = target[0];	m.m21 = target[1];	m.m22 = target[2];
		return m;
	}

	inline mat4 Perspective(float fov, int width, int heigth, float nr, float fr)
	{
		const float horFovRad = fov * 0.0174533f;
		const float vertFovRad = 2.0f * std::atanf(std::tanf(horFovRad * .5f) * ((float)heigth / (float)width));
		const float range = 1.0f / (nr - fr);

		fov = 1.f / std::tanf(vertFovRad * .5f);

		mat4 m(0);
		m.m00 = fov / ((float)width / (float)heigth);
		m.m11 = fov;
		m.m22 = (nr + fr) * range;
		m.m23 = 2.0f * fr * nr * range;
		m.m32 = -1.f;

		return m;
	}

	inline quaternion MatToQuat(const mat4& m)
	{
		float tr = m.m00 + m.m11 + m.m22;
		quaternion out;

		if (tr > 0) {
			float S = 1.f / (std::sqrtf(tr + 1.f) * 2.f); // S=4*qw 
			out.w = 0.25f / S;
			out.x = (m.m21 - m.m12) * S;
			out.y = (m.m02 - m.m20) * S;
			out.z = (m.m10 - m.m01) * S;
		}
		else if ((m.m00 > m.m11) && (m.m00 > m.m22)) {
			float S = 1.f / (std::sqrtf(1.f + m.m00 - m.m11 - m.m22) * 2.f); // S=4*qx 
			out.w = (m.m21 - m.m12) * S;
			out.x = 0.25f / S;
			out.y = (m.m01 + m.m10) * S;
			out.z = (m.m02 + m.m20) * S;
		}
		else if (m.m11 > m.m22) {
			float S = 1.f / (std::sqrtf(1.f + m.m11 - m.m00 - m.m22) * 2.f); // S=4*qy
			out.w = (m.m02 - m.m20) * S;
			out.x = (m.m01 + m.m10) * S;
			out.y = 0.25f / S;
			out.z = (m.m12 + m.m21) * S;
		}
		else {
			float S = 1.f / (std::sqrtf(1.f + m.m22 - m.m00 - m.m11) * 2.f); // S=4*qz
			out.w = (m.m10 - m.m01) * S;
			out.x = (m.m02 + m.m20) * S;
			out.y = (m.m12 + m.m21) * S;
			out.z = 0.25f / S;
		}

		return out.normalized();
	}
}