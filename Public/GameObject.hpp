#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP

#include "EnabledTag.hpp"
#include "Transform.hpp"
#include "ArenaAllocator.hpp"

class GameObjectSystem;
class GameObject
{
	friend GameObjectSystem;
private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

public:
	virtual ~GameObject() {}
	GameObject() : 
		m_reg{nullptr},
		m_entity{entt::null} {}
	GameObject(const GameObject& other) :
		m_reg{other.m_reg},
		m_entity{other.m_entity} {}
	GameObject& operator=(const GameObject& rhs) {
		if (this == &rhs) return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		return *this;
	}

#pragma warning(push)
#pragma warning(disable : 26447)
	GameObject(GameObject&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	GameObject& operator=(GameObject&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
#pragma warning(pop)

	bool operator==(const GameObject& rhs) {
		return m_reg == rhs.m_reg && m_entity == rhs.m_entity;
	}
	operator entt::entity() { return m_entity; }

	Enabled enabled() { return Enabled{ m_reg, m_entity }; }
	Transform transform() { return Transform{ m_reg, m_entity }; }

};

static_assert(std::is_nothrow_move_constructible_v<GameObject>, "GameObject is not noexcept movable!");

#endif