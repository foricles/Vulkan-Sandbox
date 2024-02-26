#pragma once
#include <cmath>

namespace math
{
	class vec4
	{
	public:
		union
		{
			float n[4];
			struct { float x; float y; float z; float w; };
		};
	public:
		vec4();
		vec4(float a, float b, float c, float d);
		vec4(const vec4 &vec);
		~vec4();

		float dot(const vec4 &vec)const;
		float magnitude()const;
		float sqrMagnitude()const;
		vec4 &normalize();
		vec4 normalized()const;

		float &operator[](size_t id);
		const vec4 &operator=(const vec4 &vec);

		vec4 &operator+=(const vec4 &vec);
		vec4 &operator-=(const vec4 &vec);
		vec4 &operator*=(float n);
		vec4 &operator/=(float n);

		vec4 operator+(const vec4 &vec)const;
		vec4 operator-(const vec4 &vec)const;
		vec4 operator*(float n)const;
		vec4 operator/(float n)const;
	};


	//==============================================================================================
	//==============================================================================================


	inline vec4::vec4() : vec4(0.0f, 0.0f, 0.0f, 1.0f) {}

	inline vec4::vec4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d){}

	inline vec4::vec4(const vec4& vec) : vec4(vec.x, vec.y, vec.z, vec.w) {}

	inline vec4::~vec4() {}

	inline float vec4::dot(const vec4& vec)const { return (x * vec.x + y * vec.y + z * vec.z + w * vec.w); }

	inline float vec4::magnitude()const { return std::sqrtf(sqrMagnitude()); }
		
	inline float vec4::sqrMagnitude() const{ return (x * x + y * y + z * z + w * w); }

	inline vec4& vec4::normalize()
	{
		const float l = 1.0f / this->magnitude();
		x *= l;
		y *= l;
		z *= l;
		w *= l;
		return *this;
	}

	inline vec4 vec4::normalized() const
	{
		const float l = 1.0f / this->magnitude();
		return vec4(x * l, y * l, z * l, w * l);
	}

	inline float& vec4::operator[](size_t id)
	{
		return n[id];
	}

	inline const vec4& vec4::operator=(const vec4& vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
		w = vec.w;
		return *this;
	}

	inline vec4& vec4::operator+=(const vec4& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
		return *this;
	}

	inline vec4& vec4::operator-=(const vec4& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
		return *this;
	}

	inline vec4& vec4::operator*=(float n)
	{
		x *= n;
		y *= n;
		z *= n;
		w *= n;
		return *this;
	}

	inline vec4& vec4::operator/=(float n)
	{
		const float l = 1.0f / n;
		x *= l;
		y *= l;
		z *= l;
		w *= l;
		return *this;
	}

	inline vec4 vec4::operator+(const vec4& vec) const
	{
		return vec4(x + vec.x,
			y + vec.y,
			z + vec.z,
			w + vec.w);
	}

	inline vec4 vec4::operator-(const vec4& vec) const
	{
		return vec4(x - vec.x,
			y - vec.y,
			z - vec.z,
			w - vec.w);
	}

	inline vec4 vec4::operator*(float n) const
	{
		return vec4(x * n, y * n, z * n, w * n);
	}

	inline vec4 vec4::operator/(float n) const
	{
		const float l = 1.0f / n;
		return vec4(x * n, y * n, z * n, w * n);
	}
}