#pragma once
#include <cmath>

namespace math
{
	class vec3
	{
	public:
		union
		{
			float n[3];
			struct { float x; float y; float z; };
		};

	public:
		vec3();
		vec3(float a, float b, float c);
		vec3(const vec3 &vec);
		~vec3();

		float magnitude()const;
		float sqrMagnitude()const;
		float dot(const vec3 &vec)const;
		vec3 cross(const vec3 &vec)const;
		vec3 &normalize();
		vec3 normalized()const;

		void set(float X, float Y, float Z);

		float &operator[](size_t id);
		const vec3 &operator=(const vec3 &vec);
		vec3 &operator+=(const vec3 &vec);
		vec3 &operator-=(const vec3 &vec);
		vec3 &operator*=(float n);
		vec3 &operator/=(float n);

		friend vec3 operator+(const vec3 &v, const vec3 &vec);
		friend vec3 operator-(const vec3 &v, const vec3 &vec);
		friend vec3 operator/(const vec3 &v, float n);
		friend vec3 operator*(const vec3 &v, float n);
		friend vec3 operator*(float n, const vec3& v) { return v * n; }
	};


	//==============================================================================================
	//==============================================================================================


	inline vec3::vec3() : vec3(0.0f, 0.0f, 0.0f) {}

	inline vec3::vec3(float a, float b, float c) : x(a), y(b), z(c) { }

	inline vec3::vec3(const vec3& vec) : vec3(vec.x, vec.y, vec.z) { }

	inline vec3::~vec3() { }

	inline float vec3::sqrMagnitude() const { return (x * x + y * y + z * z); }

	inline float vec3::magnitude()const { return std::sqrtf(sqrMagnitude()); }

	inline float vec3::dot(const vec3& vec) const { return (x * vec.x + y * vec.y + z * vec.z); }

	inline vec3 vec3::cross(const vec3& vec) const
	{
		return vec3(
			y * vec.z - z * vec.y,
			z * vec.x - x * vec.z,
			x * vec.y - y * vec.x
		);
	}

	inline vec3& vec3::normalize()
	{
		const float len = this->magnitude();
		x /= len;
		y /= len;
		z /= len;
		return *this;
	}

	inline vec3 vec3::normalized()const
	{
		const float len = this->magnitude();
		return vec3(x / len, y / len, z / len);
	}

	inline void vec3::set(float X, float Y, float Z)
	{
		x = X;
		y = Y;
		z = Z;
	}

	inline float& vec3::operator[](size_t id)
	{
		return n[id];
	}

	inline const vec3& vec3::operator=(const vec3& vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
		return *this;
	}

	inline vec3& vec3::operator+=(const vec3& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		return *this;
	}

	inline vec3& vec3::operator-=(const vec3& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		return *this;
	}

	inline vec3& vec3::operator*=(float n)
	{
		x *= n;
		y *= n;
		z *= n;
		return *this;
	}

	inline vec3& vec3::operator/=(float n)
	{
		x /= n;
		y /= n;
		z /= n;
		return *this;
	}

	inline vec3 operator+(const vec3& v, const vec3& vec)
	{
		return vec3(v.x + vec.x,
			v.y + vec.y,
			v.z + vec.z);
	}

	inline vec3 operator-(const vec3& v, const vec3& vec)
	{
		return vec3(v.x - vec.x,
			v.y - vec.y,
			v.z - vec.z);
	}

	inline vec3 operator*(const vec3& v, float n)
	{
		return vec3(v.x * n,
			v.y * n,
			v.z * n);
	}

	inline vec3 operator/(const vec3& v, float n)
	{
		return vec3(v.x / n,
			v.y / n,
			v.z / n);
	}
}