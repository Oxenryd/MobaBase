#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Bits.hpp"

struct TransformComponent
{
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
};

enum class ModelState : uint16_t
{
	None		= 0x0000,
	Enabled		= 0x0001,
	DirtyMatrix = 0x0002
};

struct ModelStateComponent
{
	uint32_t matrixIndex;
	SizedBitField<uint16_t> state;
};

#endif