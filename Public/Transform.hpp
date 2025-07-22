#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Bits.hpp"
#include <entt/entt.hpp>
#include "MobaMath.hpp"
#include "ObjectState.hpp"
#include "GlobalMacros.h"

struct TransformComponent
{
	uint32_t matrixIndex = UINT32_INVALID;
	glm::vec3 position{};
	glm::quat rotation{};
	glm::vec3 scale{};
	SizedBitField<uint8_t> state{ObjectState::Enabled};

	glm::mat4x4 trs() {
		return MMath::composeTRS(position, rotation, scale);
	}
};

class Transform
{
private:
	

public:
};
#endif