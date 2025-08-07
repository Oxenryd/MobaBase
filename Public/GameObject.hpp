#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP

#include "EnabledTag.hpp"
#include "Transform.hpp"
#include "ArenaAllocator.hpp"
#include "GlobalComponents.hpp"

class GameObjectSystem;
class GlobalSystem;
class Engine;
class GameObject
{
	friend GameObjectSystem;
	friend GlobalSystem;
	friend Engine;

protected:
	ArenaRegistry* m_reg;
	entt::entity m_entity;
	entt::entity m_globalEntity;

public:
	virtual ~GameObject() {}
	GameObject() : 
		m_reg{nullptr},
		m_entity{entt::null},
		m_globalEntity{ entt::null } {}
	GameObject(const GameObject& other) :
		m_reg{other.m_reg},
		m_entity{other.m_entity},
		m_globalEntity{ other.m_globalEntity } {}
	GameObject& operator=(const GameObject& rhs) {
		if (this == &rhs) return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		m_globalEntity = rhs.m_globalEntity;
		return *this;
	}

#pragma warning(push)
#pragma warning(disable : 26447)
	GameObject(GameObject&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;
		m_globalEntity = other.m_globalEntity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
		other.m_globalEntity = entt::null;
	}
	GameObject& operator=(GameObject&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;
		m_globalEntity = other.m_globalEntity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
		other.m_globalEntity = entt::null;
	}
#pragma warning(pop)

	bool operator==(const GameObject& rhs) {
		return m_reg == rhs.m_reg && m_entity == rhs.m_entity && m_globalEntity == rhs.m_globalEntity;
	}
	operator entt::entity() { return m_entity; }

	INLINE Enabled enabled() { return Enabled{ m_reg, m_entity }; }
	INLINE Enabled enabled() const { return Enabled{ m_reg, m_entity }; }
	INLINE Transform transform() { return Transform{ m_reg, m_entity }; }
	INLINE Transform transform() const { return Transform{ m_reg, m_entity }; }
	INLINE Layer layer() { return Layer{ m_globalEntity }; }
	INLINE Layer layer() const { return Layer{ m_globalEntity }; }

	INLINE entt::entity entity() const { return m_entity; }
};

static_assert(std::is_nothrow_move_constructible_v<GameObject>, "GameObject is not noexcept movable!");

#endif