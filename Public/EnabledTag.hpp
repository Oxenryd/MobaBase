#ifndef ENABLEDTAG_HPP
#define ENABLEDTAG_HPP

#include "ArenaAllocator.hpp"

struct EnabledTag{
private:
	uint8_t _raw = 0;
};

class Enabled
{
private:
	entt::entity m_entity;
	ArenaRegistry* m_reg;

public:
	Enabled() :
		m_reg{ nullptr },
		m_entity{ entt::null } {}
	Enabled(ArenaRegistry* registry, entt::entity entity) :
		m_reg{registry},
		m_entity{entity} {}
	Enabled(const Enabled& other) :
		m_reg{ other.m_reg },
		m_entity{ other.m_entity } {}
	Enabled& operator=(const Enabled& rhs) {
		if (this == &rhs) return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		return *this;
	}
	Enabled(Enabled&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	Enabled& operator=(Enabled&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	bool operator==(const Enabled& rhs) {
		return 		m_reg == rhs.m_reg &&
					m_entity == rhs.m_entity;
	}
	bool operator==(const bool& rhs) {
		return status() == rhs;
	}
	operator entt::entity() { return m_entity; }

	void set(bool enabledValue) {
		EnabledTag* tag = m_reg->try_get<EnabledTag>(m_entity);
		if (enabledValue) {
			if (!tag) m_reg->emplace<EnabledTag>(m_entity);
		} else {
			if (tag) m_reg->remove<EnabledTag>(m_entity);
		}
	}

	bool status() const {
		return m_reg->try_get<EnabledTag>(m_entity) != nullptr;
	}

};

static_assert(std::is_nothrow_move_constructible_v<Enabled>, "Enabled is not noexcept");

#endif