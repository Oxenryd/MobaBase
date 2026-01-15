#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <glm/glm.hpp>

struct FrustumPlane
{
	union
	{
		float raw[4]{};
		struct
		{
			float normal[3];
			float d;
		};
		struct
		{
			float x, y, z;
		};
		glm::vec4 vec;
	};
};

struct Frustum
{
	FrustumPlane planes[6];
	enum { Left, Right, Bottom, Top, Near, Far };
};



#endif