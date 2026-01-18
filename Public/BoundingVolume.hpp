#ifndef BOUNDING_VOLUME_HPP
#define BOUNDING_VOLUME_HPP

#include <entt/entt.hpp>
#include "ArenaAllocator.hpp"
#include "BasicTypes.hpp"

struct BoundingVolume
{
private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

public:
	~BoundingVolume() = default;
	BoundingVolume() : 
		m_reg{nullptr},
		m_entity{entt::null}
	{}

	BoundingVolume(ArenaRegistry* const registry, const entt::entity entity)
		: m_reg{ registry }, m_entity{ entity } {}
	BoundingVolume(const BoundingVolume& other) {
		m_reg = other.m_reg;
		m_entity = other.m_entity;
	}
	BoundingVolume& operator=(const BoundingVolume& rhs) {
		if (this == &rhs)
			return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		return *this;
	}
	BoundingVolume(BoundingVolume&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	BoundingVolume& operator=(BoundingVolume&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;

		return *this;
	}
	bool operator==(const BoundingVolume& rhs) const {
		return m_reg == rhs.m_reg && m_entity == rhs.m_entity;
	}

	void setFlags(BoundingVolumeFlags flags);

	void setCoarseAABB(const AABB& aabb);
	void setCoarseAABB_local(const AABB& aabb);
	AABB getCoarseAABB() const;
	AABB getCoarseAABB_local() const;
	
	INLINE operator BoundingVolumeComponent& () const {
		return m_reg->get<BoundingVolumeComponent>(m_entity);
	}
	INLINE operator const BoundingVolumeComponent& () const {
		return m_reg->get<BoundingVolumeComponent>(m_entity);
	}

};

#endif