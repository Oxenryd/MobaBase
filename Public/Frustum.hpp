#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <glm/glm.hpp>

#include "glm/gtc/type_ptr.hpp"

struct FrustumPlane
{
	// [0..2] normal
	// [3] D
	glm::vec4 raw;

	[[nodiscard]] INLINE glm::vec3 normal() const { return xyz(raw); }
	[[nodiscard]] INLINE float d() const { return raw.w; }
};

struct Frustum
{
	FrustumPlane planes[6];
	enum { Left, Right, Bottom, Top, Near, Far };
};



#endif