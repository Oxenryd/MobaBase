#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <entt/entt.hpp>
#include <cstdint>

#include "Bits.hpp"

enum class TransformState : uint8_t
{
	None		= 0,
	Enabled		= 1 << 0,
	Static		= 1 << 1
};

struct TransformComponent
{
	glm::vec4 position;
	glm::vec4 scale = { 1.0f, 1.0f, 1.0f, 0.0f };
	glm::quat rotation;
	SizedBitField<uint8_t> state = { 1 };
};

struct Transform
{
public:
	~Transform() = default;
	Transform() : 
		entity {entt::null} {}
	Transform(const entt::entity& entity) :
		entity{entity} {}
	Transform(const Transform& other) : 
		entity{other.entity} {}
	Transform& operator=(const Transform& other) {
		entity = other.entity;
		return *this;
	}
private:
	entt::entity entity;
};

class TransFormSystem
{
	// USES SIMD TO CREATE ALL MODEL MATRICES AND PACK THEM INTO A BUFFER ACCESSIBLE FOR THE LATER STAGES
};

#endif