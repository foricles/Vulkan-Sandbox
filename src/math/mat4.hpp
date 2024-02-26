#pragma once

#include <cstdint>
#include "vec3.hpp"
#include "vec4.hpp"


namespace math
{
	struct mat4
	{
		float m00, m01, m02, m03;
		float m10, m11, m12, m13;
		float m20, m21, m22, m23;
		float m30, m31, m32, m33;

		mat4();
		mat4(float main_diagonal);
		mat4(const vec3& main_diagonal);
		mat4(const vec4& main_diagonal);
		mat4(float _m00, float _m01, float _m02, float _m03, float _m10, float _m11, float _m12, float _m13, float _m20, float _m21, float _m22, float _m23, float _m30, float _m31, float _m32, float _m33);

		mat4(mat4&&) = default;
		mat4(const mat4&) = default;
		mat4& operator=(mat4&&) = default;
		mat4& operator=(const mat4&) = default;

		void		identity();
		float		det() const;
		mat4&		transpose();
		mat4		transposed() const;
		mat4&		invert();
		mat4		inverted() const;
		vec3		scale() const;
		vec3		position() const;
		vec3		euler() const;

		float&		operator[](uint32_t el);
		float		operator[](uint32_t el) const;
		float&		operator()(uint32_t i, uint32_t j);
		float		operator()(uint32_t i, uint32_t j) const;

		void		operator*=(float n);
		void		operator/=(float n);
		void		operator+=(const mat4& rhv);
		void		operator-=(const mat4& rhv);

		friend mat4 operator*(const mat4& lhv, const mat4& rhv);
		friend vec4 operator*(const mat4& lhv, const vec4& rhv);
	};


	//==============================================================================================
	//==============================================================================================


	inline mat4::mat4()
		: m00(0), m01(0), m02(0), m03(0)
		, m10(0), m11(0), m12(0), m13(0)
		, m20(0), m21(0), m22(0), m23(0)
		, m30(0), m31(0), m32(0), m33(0)
	{}
		
	inline mat4::mat4(float main_diagonal)
		: m00(main_diagonal), m01(0), m02(0), m03(0)
		, m10(0), m11(main_diagonal), m12(0), m13(0)
		, m20(0), m21(0), m22(main_diagonal), m23(0)
		, m30(0), m31(0), m32(0), m33(main_diagonal)
	{}
		
	inline mat4::mat4(const vec3& main_diagonal)
		: m00(main_diagonal.x), m01(0), m02(0), m03(0)
		, m10(0), m11(main_diagonal.y), m12(0), m13(0)
		, m20(0), m21(0), m22(main_diagonal.z), m23(0)
		, m30(0), m31(0), m32(0), m33(1.0f)
	{}
		
	inline mat4::mat4(const vec4& main_diagonal)
		: m00(main_diagonal.x), m01(0), m02(0), m03(0)
		, m10(0), m11(main_diagonal.y), m12(0), m13(0)
		, m20(0), m21(0), m22(main_diagonal.z), m23(0)
		, m30(0), m31(0), m32(0), m33(main_diagonal.w)
	{}

	inline mat4::mat4(float _m00, float _m01, float _m02, float _m03, float _m10, float _m11, float _m12, float _m13, float _m20, float _m21, float _m22, float _m23, float _m30, float _m31, float _m32, float _m33)
		: m00(_m00), m01(_m01), m02(_m02), m03(_m03)
		, m10(_m10), m11(_m11), m12(_m12), m13(_m13)
		, m20(_m20), m21(_m21), m22(_m22), m23(_m23)
		, m30(_m30), m31(_m31), m32(_m32), m33(_m33)
	{}
		

	inline void mat4::identity()
	{
		m00 = 1; m01 = 0; m02 = 0; m03 = 0;
		m10 = 0; m11 = 1; m12 = 0; m13 = 0;
		m20 = 0; m21 = 0; m22 = 1; m23 = 0;
		m30 = 0; m31 = 0; m32 = 0; m33 = 1;
	}
		
	inline float mat4::det() const
	{
		return m00*(m11*m22*m33 - m11*m32*m23 - m12*m21*m33 + m12*m31*m23 + m13*m21*m32 - m13*m31*m22)
				- m01*(m10*m22*m33 - m10*m32*m23 - m12*m20*m33 + m12*m30*m23 + m13*m20*m32 - m13*m30*m22)
				+ m02*(m10*m21*m33 - m10*m31*m23 - m11*m20*m33 + m11*m30*m23 + m13*m20*m31 - m13*m30*m21)
				- m03*(m10*m21*m32 - m10*m31*m22 - m11*m20*m32 + m11*m30*m22 + m12*m20*m31 - m12*m30*m21);
	}
		
	inline mat4& mat4::transpose()
	{
		mat4 trs(
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33
		);
		*this = trs;
		return *this;
	}
		
	inline mat4 mat4::transposed() const
	{
		mat4 trs(
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33
		);
		return trs;
	}
		
	inline mat4& mat4::invert()
	{
		const double s0 =  (double)((*this)[0]);  const double s1 =  (double)((*this)[1]);  const double s2 =  (double)((*this)[2]);  const double s3 =  (double)((*this)[3]);
		const double s4 =  (double)((*this)[4]);  const double s5 =  (double)((*this)[5]);  const double s6 =  (double)((*this)[6]);  const double s7 =  (double)((*this)[7]);
		const double s8 =  (double)((*this)[8]);  const double s9 =  (double)((*this)[9]);  const double s10 = (double)((*this)[10]); const double s11 = (double)((*this)[11]);
		const double s12 = (double)((*this)[12]); const double s13 = (double)((*this)[13]); const double s14 = (double)((*this)[14]); const double s15 = (double)((*this)[15]);

		double inv[16];
		inv[0] = s5 * s10 * s15 - s5 * s11 * s14 - s9 * s6 * s15 + s9 * s7 * s14 + s13 * s6 * s11 - s13 * s7 * s10;
		inv[1] = -s1 * s10 * s15 + s1 * s11 * s14 + s9 * s2 * s15 - s9 * s3 * s14 - s13 * s2 * s11 + s13 * s3 * s10;
		inv[2] = s1 * s6 * s15 - s1 * s7 * s14 - s5 * s2 * s15 + s5 * s3 * s14 + s13 * s2 * s7 - s13 * s3 * s6;
		inv[3] = -s1 * s6 * s11 + s1 * s7 * s10 + s5 * s2 * s11 - s5 * s3 * s10 - s9 * s2 * s7 + s9 * s3 * s6;
		inv[4] = -s4 * s10 * s15 + s4 * s11 * s14 + s8 * s6 * s15 - s8 * s7 * s14 - s12 * s6 * s11 + s12 * s7 * s10;
		inv[5] = s0 * s10 * s15 - s0 * s11 * s14 - s8 * s2 * s15 + s8 * s3 * s14 + s12 * s2 * s11 - s12 * s3 * s10;
		inv[6] = -s0 * s6 * s15 + s0 * s7 * s14 + s4 * s2 * s15 - s4 * s3 * s14 - s12 * s2 * s7 + s12 * s3 * s6;
		inv[7] = s0 * s6 * s11 - s0 * s7 * s10 - s4 * s2 * s11 + s4 * s3 * s10 + s8 * s2 * s7 - s8 * s3 * s6;
		inv[8] = s4 * s9 * s15 - s4 * s11 * s13 - s8 * s5 * s15 + s8 * s7 * s13 + s12 * s5 * s11 - s12 * s7 * s9;
		inv[9] = -s0 * s9 * s15 + s0 * s11 * s13 + s8 * s1 * s15 - s8 * s3 * s13 - s12 * s1 * s11 + s12 * s3 * s9;
		inv[10] = s0 * s5 * s15 - s0 * s7 * s13 - s4 * s1 * s15 + s4 * s3 * s13 + s12 * s1 * s7 - s12 * s3 * s5;
		inv[11] = -s0 * s5 * s11 + s0 * s7 * s9 + s4 * s1 * s11 - s4 * s3 * s9 - s8 * s1 * s7 + s8 * s3 * s5;
		inv[12] = -s4 * s9 * s14 + s4 * s10 * s13 + s8 * s5 * s14 - s8 * s6 * s13 - s12 * s5 * s10 + s12 * s6 * s9;
		inv[13] = s0 * s9 * s14 - s0 * s10 * s13 - s8 * s1 * s14 + s8 * s2 * s13 + s12 * s1 * s10 - s12 * s2 * s9;
		inv[14] = -s0 * s5 * s14 + s0 * s6 * s13 + s4 * s1 * s14 - s4 * s2 * s13 - s12 * s1 * s6 + s12 * s2 * s5;
		inv[15] = s0 * s5 * s10 - s0 * s6 * s9 - s4 * s1 * s10 + s4 * s2 * s9 + s8 * s1 * s6 - s8 * s2 * s5;

		double det = s0 * inv[0] + s1 * inv[4] + s2 * inv[8] + s3 * inv[12];
		if (det != 0.0)
		{
			det = 1.0 / det;
		}
		for (int i = 0; i < 16; i++)
		{
			(*this)[i] = (float)(inv[i] * det);
		}
		return *this;
	}

	inline mat4 mat4::inverted() const
	{
		mat4 result = *this;
		result.invert();
		return result;
	}

	inline vec3 mat4::scale() const
	{
		math::vec3 ret;

		ret.x = math::vec3(m00, m10, m20).magnitude();
		ret.y = math::vec3(m01, m11, m21).magnitude();
		ret.z = math::vec3(m02, m12, m22).magnitude();

		return ret;
	}

	inline vec3 mat4::position() const
	{
		return vec3(m03, m13, m23);
	}

	inline vec3 mat4::euler() const
	{
		float x(0), y(0), z(0);
		const float sy = ::sqrtf(m00 * m00 + m10 * m10 );
		const bool singular = sy < 1e-6;
		if (!singular)
		{
			x = std::atan2f(m21 , m22);
			y = std::atan2f(-m20, sy);
			z = std::atan2f(m10, m00);
		}
		else
		{
			x = std::atan2f(-m12, m11);
			y = std::atan2f(-m20, sy);
			z = 0;
		}
		return vec3(x, y, z);
	}
		

	inline float& mat4::operator[](uint32_t el)
	{
		return *(&m00 + el);
	}
		
	inline float mat4::operator[](uint32_t el) const
	{
		return *(&m00 + el);
	}
		
	inline float& mat4::operator()(uint32_t i, uint32_t j)
	{
		return *(&m00 + (i * 4 + j));
	}
		
	inline float mat4::operator()(uint32_t i, uint32_t j) const
	{
		return *(&m00 + (i * 4 + j));
	}
		

	inline void mat4::operator*=(float n)
	{
		m00 *= n; m01 *= n; m02 *= n; m03 *= n;
		m10 *= n; m11 *= n; m12 *= n; m13 *= n;
		m20 *= n; m21 *= n; m22 *= n; m23 *= n;
		m30 *= n; m31 *= n; m32 *= n; m33 *= n;
	}
		
	inline void mat4::operator/=(float n)
	{
		const float l = 1.0f / n;
		m00 *= l; m01 *= l; m02 *= l; m03 *= l;
		m10 *= l; m11 *= l; m12 *= l; m13 *= l;
		m20 *= l; m21 *= l; m22 *= l; m23 *= l;
		m30 *= l; m31 *= l; m32 *= l; m33 *= l;
	}
		
	inline void mat4::operator+=(const mat4& rhv)
	{
		m00 += rhv.m00; m01 += rhv.m01; m02 += rhv.m02; m03 += rhv.m03;
		m10 += rhv.m10; m11 += rhv.m11; m12 += rhv.m12; m13 += rhv.m13;
		m20 += rhv.m20; m21 += rhv.m21; m22 += rhv.m22; m23 += rhv.m23;
		m30 += rhv.m30; m31 += rhv.m31; m32 += rhv.m32; m33 += rhv.m33;
	}
		
	inline void mat4::operator-=(const mat4& rhv)
	{
		m00 -= rhv.m00; m01 -= rhv.m01; m02 -= rhv.m02; m03 -= rhv.m03;
		m10 -= rhv.m10; m11 -= rhv.m11; m12 -= rhv.m12; m13 -= rhv.m13;
		m20 -= rhv.m20; m21 -= rhv.m21; m22 -= rhv.m22; m23 -= rhv.m23;
		m30 -= rhv.m30; m31 -= rhv.m31; m32 -= rhv.m32; m33 -= rhv.m33;
	}
		

	inline mat4 operator*(const mat4& lhv, const mat4& rhv)
	{
		mat4 r;
		r.m00 = lhv.m00*rhv.m00 + lhv.m01*rhv.m10 + lhv.m02*rhv.m20 + lhv.m03*rhv.m30;
		r.m01 = lhv.m00*rhv.m01 + lhv.m01*rhv.m11 + lhv.m02*rhv.m21 + lhv.m03*rhv.m31;
		r.m02 = lhv.m00*rhv.m02 + lhv.m01*rhv.m12 + lhv.m02*rhv.m22 + lhv.m03*rhv.m32;
		r.m03 = lhv.m00*rhv.m03 + lhv.m01*rhv.m13 + lhv.m02*rhv.m23 + lhv.m03*rhv.m33;

		r.m10 = lhv.m10*rhv.m00 + lhv.m11*rhv.m10 + lhv.m12*rhv.m20 + lhv.m13*rhv.m30;
		r.m11 = lhv.m10*rhv.m01 + lhv.m11*rhv.m11 + lhv.m12*rhv.m21 + lhv.m13*rhv.m31;
		r.m12 = lhv.m10*rhv.m02 + lhv.m11*rhv.m12 + lhv.m12*rhv.m22 + lhv.m13*rhv.m32;
		r.m13 = lhv.m10*rhv.m03 + lhv.m11*rhv.m13 + lhv.m12*rhv.m23 + lhv.m13*rhv.m33;

		r.m20 = lhv.m20*rhv.m00 + lhv.m21*rhv.m10 + lhv.m22*rhv.m20 + lhv.m23*rhv.m30;
		r.m21 = lhv.m20*rhv.m01 + lhv.m21*rhv.m11 + lhv.m22*rhv.m21 + lhv.m23*rhv.m31;
		r.m22 = lhv.m20*rhv.m02 + lhv.m21*rhv.m12 + lhv.m22*rhv.m22 + lhv.m23*rhv.m32;
		r.m23 = lhv.m20*rhv.m03 + lhv.m21*rhv.m13 + lhv.m22*rhv.m23 + lhv.m23*rhv.m33;

		r.m30 = lhv.m30*rhv.m00 + lhv.m31*rhv.m10 + lhv.m32*rhv.m20 + lhv.m33*rhv.m30;
		r.m31 = lhv.m30*rhv.m01 + lhv.m31*rhv.m11 + lhv.m32*rhv.m21 + lhv.m33*rhv.m31;
		r.m32 = lhv.m30*rhv.m02 + lhv.m31*rhv.m12 + lhv.m32*rhv.m22 + lhv.m33*rhv.m32;
		r.m33 = lhv.m30*rhv.m03 + lhv.m31*rhv.m13 + lhv.m32*rhv.m23 + lhv.m33*rhv.m33;
		return r;
	}
		
	inline vec4 operator*(const mat4& lhv, const vec4& rhv)
	{
		return vec4(
			lhv.m00 * rhv.x + lhv.m01 * rhv.y + lhv.m02 * rhv.z + lhv.m03 * rhv.w,
			lhv.m10 * rhv.x + lhv.m11 * rhv.y + lhv.m12 * rhv.z + lhv.m13 * rhv.w,
			lhv.m20 * rhv.x + lhv.m21 * rhv.y + lhv.m22 * rhv.z + lhv.m23 * rhv.w,
			lhv.m30 * rhv.x + lhv.m31 * rhv.y + lhv.m32 * rhv.z + lhv.m33 * rhv.w
		);
	}
}