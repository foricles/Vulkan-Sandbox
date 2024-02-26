#pragma once
#include "../math/mat4.hpp"
#include "../math/vec3.hpp"
#include "../math/matrices.hpp"
#include "../math/quaternion.hpp"

class Camera
{
public:
	Camera()
		: m_pos(0, 0, 0)
		, m_rot(0, 0, 0, 1)
		, m_proj(1)
	{
	}
	~Camera()
	{
	}

	void SetRotation(const math::vec3& euler)											{ m_rot = math::quaternion::euler(euler); }
	void PerspectiveProjection(float fov, uint32_t w, uint32_t h, float nr, float fr)	{ m_proj = math::Perspective(fov, w, h, nr, fr); }

	math::mat4 View() const									{ return math::CameraMatrix(Forward(), Up()) * math::Translation(-m_pos.x, -m_pos.y, -m_pos.z); }
	math::vec3 Up() const									{ return m_rot.apply_rot(math::vec3(0, 1, 0)); }
	math::vec3 Forward() const								{ return m_rot.apply_rot(math::vec3(0, 0, 1)); }
	math::vec3 TransformDirection(const math::vec3& orig)	{ return m_rot.apply_rot(orig); }
	void Translate(const math::vec3& tr)					{ m_pos += TransformDirection(tr); }

public:
	math::vec3& Position()					{ return m_pos; }
	const math::mat4& Projection() const	{ return m_proj; }

private:
	math::vec3 m_pos;
	math::quaternion m_rot;
	math::mat4 m_proj;
};