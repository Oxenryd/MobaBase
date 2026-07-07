#ifndef ENABLEDTAG_HPP
#define ENABLEDTAG_HPP

#include "ArenaAllocator.hpp"
#include "Transform.hpp"

struct EnabledTag{
private:
	uint8_t _raw = 0;
};

struct Enabled
{
private:
	entt::entity m_entity;
	ArenaRegistry* m_reg;

public:
	INLINE Enabled() :
		m_reg{ nullptr },
		m_entity{ entt::null } {}
	INLINE Enabled(ArenaRegistry* registry, entt::entity entity) :
		m_reg{registry},
		m_entity{entity} {}
	INLINE Enabled(const Enabled& other) :
		m_reg{ other.m_reg },
		m_entity{ other.m_entity } {}
	INLINE Enabled& operator=(const Enabled& rhs) {
		if (this == &rhs) return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		return *this;
	}
	INLINE Enabled(Enabled&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	INLINE Enabled& operator=(Enabled&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	INLINE bool operator==(const Enabled& rhs) {
		return 		m_reg == rhs.m_reg &&
					m_entity == rhs.m_entity;
	}
	INLINE bool operator==(const bool& rhs) {
		return status() == rhs;
	}
	INLINE operator entt::entity() { return m_entity; }

	INLINE void set(bool enabledValue) {
		EnabledTag* tag = m_reg->try_get<EnabledTag>(m_entity);
		if (enabledValue) {
			if (!tag) { 
				m_reg->emplace<EnabledTag>(m_entity);
			}
		} else {
			if (tag) { 
				m_reg->remove<EnabledTag>(m_entity);
			}
		}

		auto transform = m_reg->try_get<TransformComponent>(m_entity);
		if (transform) {
			if (transform->state.hasFlag(ObjectState::LocalEnableOverride))
				return;

			std::span<entt::entity> children = Transform::getChildrenEntities(m_reg, m_entity);
			for (auto& entity : children) {
				Enabled childEnabled{ m_reg, entity };
				childEnabled.set(enabledValue);
			}
			

		}
	}

	INLINE bool status() const {
		return m_reg->try_get<EnabledTag>(m_entity) != nullptr;
	}

};

static_assert(std::is_nothrow_move_constructible_v<Enabled>, "Enabled is not noexcept");

namespace entt
{
	template<>
	struct storage_type<EnabledTag, ArenaRegistry>
	{
		using type = ArenaStorage<EnabledTag>;
	};
}

#endif