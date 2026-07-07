#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <glm/glm.hpp>
#include "HlslTypes.h"

#include "glm/gtc/type_ptr.hpp"

struct FrustumPlane
{
	union
	{
		float raw[4];
		struct {
			FArray3 m_normal;
			float m_d;
		};
	};

	float* data() {return raw;}
	const float *data() const {return raw;}

	[[nodiscard]] INLINE const glm::vec3& normal() const { return m_normal.asGlmVec(); }//xyz(raw); }
	[[nodiscard]] INLINE float d() const { return m_d; }
};

struct Frustum
{
	FrustumPlane planes[6];
	enum { Left, Right, Bottom, Top, Near, Far };
};



#endif