#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Bits.hpp"
#include "ArenaAllocator.hpp"
#include "MobaMath.hpp"
#include "ObjectState.hpp"
#include "GlobalMacros.h"

struct TransformComponent
{
	
	uint32_t matrixIndex = UINT32_INVALID;
	glm::vec3 position{};
	glm::quat rotation{};
	glm::vec3 scale{1,1,1};
	SizedBitField<uint8_t> state{0};

	glm::mat4x4 trs() {
		return MMath::composeTRS(position, rotation, scale);
	}
};

class Transform
{
	
private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

	INLINE TransformComponent& _markDirty() {
		auto& comp = m_reg->get<TransformComponent>(m_entity);
		comp.state.setByEnum(ObjectState::DirtyTransform);
		return comp;
	}

public:
	~Transform() = default;
	Transform() = delete;
	Transform(ArenaRegistry* registry, entt::entity entity)
		: m_reg{registry}, m_entity{entity} {}
	Transform(const Transform& other) {
		m_reg = other.m_reg;
		m_entity = other.m_entity;
	}
	Transform& operator=(const Transform& rhs) {
		if (this == &rhs)
			return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		return *this;
	}
	Transform(Transform&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	Transform& operator=(Transform&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	bool operator==(const Transform& rhs) {
		return m_reg == rhs.m_reg && m_entity == rhs.m_entity;
	}

	INLINE operator TransformComponent& () {
		return m_reg->get<TransformComponent>(m_entity);
	}
	INLINE operator const TransformComponent& () const {
		return m_reg->get<TransformComponent>(m_entity);
	}
	
	INLINE glm::vec3& modifyPosition() { 
		return _markDirty().position;
	}
	INLINE glm::quat& modifyRotation() {
		return _markDirty().rotation;
	}
	INLINE glm::vec3& modifyScale() {
		return _markDirty().scale;
	}
	INLINE SizedBitField<uint8_t>& modifyState() {
		return _markDirty().state;
	}

	INLINE const glm::vec3& position() const { return m_reg->get<TransformComponent>(m_entity).position; }
	INLINE const glm::vec3& scale() const { return m_reg->get<TransformComponent>(m_entity).scale; }
	INLINE const glm::quat& rotation() const { return m_reg->get<TransformComponent>(m_entity).rotation; }
	INLINE const SizedBitField<uint8_t>& state() const { return m_reg->get<TransformComponent>(m_entity).state; }

	INLINE const glm::mat4x4 trs() const { return m_reg->get<TransformComponent>(m_entity).trs(); }

	bool isValid() const {
		return m_reg && m_reg->valid(m_entity) && m_reg->all_of<TransformComponent>(m_entity);
	}


	// Static
	INLINE static glm::vec3& position(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).position;
	}
	INLINE static glm::vec3& scale(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).scale;
	}
	INLINE static glm::quat& rotation(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).rotation;
	}
	INLINE static SizedBitField<uint8_t>& state(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).state;
	}

};

#endif