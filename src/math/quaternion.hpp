#pragma once

#include "vec4.hpp"
#include "vec3.hpp"

namespace math
{
	class quaternion : public vec4
	{
	public:
		quaternion();
		quaternion(float a, float b, float c, float d);
		quaternion(const vec4 &vec);
		quaternion(const quaternion &qut);
		~quaternion();

		void conjugate();
		quaternion conjugated() const;
		quaternion &operator=(const quaternion &qut);
		quaternion &operator*=(const quaternion &qut);

		vec3 rotate(const vec3 &vect) const;
		vec3 apply_rot(const vec3& vec) const;

		vec3 to_euler() const;

		static quaternion euler(float angle, float X, float Y, float Z);
		static quaternion euler(float angle, const vec3 &axes);
		static quaternion euler(const vec3& axes);
		static quaternion euler(float yaw, float pitch, float roll);

		friend quaternion operator*(const quaternion &q1, const quaternion &q2);
		friend quaternion operator*(const quaternion &q1, const vec3 &vec);
	};


	//==============================================================================================
	//==============================================================================================


	inline quaternion::quaternion() : vec4(0, 0, 0, 1) {}

	inline quaternion::quaternion(float a, float b, float c, float d) : vec4(a, b, c, d) {}

	inline quaternion::quaternion(const vec4& vec) : vec4(vec.x, vec.y, vec.z, vec.w) {}

	inline quaternion::quaternion(const quaternion& qut) : vec4(qut.x, qut.y, qut.z, qut.w) {}

	inline quaternion::~quaternion() {}

	inline void quaternion::conjugate()
	{
		x = -x;
		y = -y;
		z = -z;
	}

	inline quaternion quaternion::conjugated() const { return quaternion(-x, -y, -z, w); }

	inline quaternion& quaternion::operator=(const quaternion& qut)
	{
		x = qut.x;
		y = qut.y;
		z = qut.z;
		w = qut.w;
		return *this;
	}

	inline quaternion& quaternion::operator*=(const quaternion& qut)
	{
		*this = (*this) * qut;
		return *this;
	}

	inline vec3 quaternion::rotate(const vec3& vect) const
	{
		auto temp = ((*this) * vect) * this->conjugated();
		return vec3(temp.x, temp.y, temp.z);
	}

	inline vec3 quaternion::apply_rot(const vec3& vec) const
	{
		const vec3 u(x, y, z);
		return 2.0f * u.dot(vec) * u
			+ (w * w - u.dot(u)) * vec
			+ 2.0f * w * u.cross(vec);
	}

	inline vec3 quaternion::to_euler() const
	{
		const double sinr_cosp = 2.0 * ((double)w * (double)x + (double)y * (double)z);
		const double cosr_cosp = 1.0 - 2.0 * ((double)x * (double)x + (double)y * (double)y);
		const double roll = std::atan2(sinr_cosp, cosr_cosp);

		const double sinp = std::sqrt(1.0 + 2.0 * ((double)w * (double)y - (double)x * (double)z));
		const double cosp = std::sqrt(1.0 - 2.0 * ((double)w * (double)y - (double)x * (double)z));
		const double pitch = 2 * std::atan2(sinp, cosp) - M_PI / 2;

		const double siny_cosp = 2.0 * ((double)w * (double)z + (double)x * (double)y);
		const double cosy_cosp = 1.0 - 2.0 * ((double)y * (double)y + (double)z * (double)z);
		const double yaw = std::atan2(siny_cosp, cosy_cosp);

		return { static_cast<float>(roll), static_cast<float>(pitch), static_cast<float>(yaw) };
	}

	inline quaternion quaternion::euler(float angle, float X, float Y, float Z)
	{
		return quaternion::euler(angle, vec3(X, Y, Z));
	}

	inline quaternion quaternion::euler(float angle, const vec3& axes)
	{
		const double hs = std::sin(static_cast<double>(angle) * 0.5);
		const double hc = std::cos(static_cast<double>(angle) * 0.5);
		const double X = axes.x * hs;
		const double Y = axes.y * hs;
		const double Z = axes.z * hs;
		return quaternion(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(hc));
	}

	inline quaternion quaternion::euler(const vec3& axes)
	{
		return euler(axes.x, axes.y, axes.z);
	}

	inline quaternion quaternion::euler(float x, float y, float z)
	{
		const double cy = std::cos(static_cast<double>(z) * 0.5);
		const double sy = std::sin(static_cast<double>(z) * 0.5);
		const double cp = std::cos(static_cast<double>(y) * 0.5);
		const double sp = std::sin(static_cast<double>(y) * 0.5);
		const double cr = std::cos(static_cast<double>(x) * 0.5);
		const double sr = std::sin(static_cast<double>(x) * 0.5);

		quaternion q;
		q.w = static_cast<float>(cr * cp * cy + sr * sp * sy);
		q.x = static_cast<float>(sr * cp * cy - cr * sp * sy);
		q.y = static_cast<float>(cr * sp * cy + sr * cp * sy);
		q.z = static_cast<float>(cr * cp * sy - sr * sp * cy);

		return q;
	}

	inline quaternion operator*(const quaternion& q1, const quaternion& q2)
	{
		const double x1 = (double)q1.x;
		const double x2 = (double)q2.x;
		const double y1 = (double)q1.y;
		const double y2 = (double)q2.y;
		const double z1 = (double)q1.z;
		const double z2 = (double)q2.z;
		const double w1 = (double)q1.w;
		const double w2 = (double)q2.w;

		const double X = x1 * w2 + w1 * x2 + y1 * z2 - z1 * y2;
		const double Y = y1 * w2 + w1 * y2 + z1 * x2 - x1 * z2;
		const double Z = z1 * w2 + w1 * z2 + x1 * y2 - y1 * x2;
		const double W = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
		return quaternion(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(W));
	}

	inline quaternion operator*(const quaternion& q1, const vec3& vec)
	{
		const double W = (double)-q1.x * (double)vec.x - (double)q1.y * (double)vec.y - (double)q1.z * (double)vec.z;
		const double X = (double)q1.w * (double)vec.x + (double)q1.y * (double)vec.z - (double)q1.z * (double)vec.y;
		const double Y = (double)q1.w * (double)vec.y + (double)q1.z * (double)vec.x - (double)q1.x * (double)vec.z;
		const double Z = (double)q1.w * (double)vec.z + (double)q1.x * (double)vec.y - (double)q1.y * (double)vec.x;
		return quaternion(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(W));
	}
}