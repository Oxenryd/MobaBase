#ifndef GLOBALCOMPONENTS_HPP
#define GLOBALCOMPONENTS_HPP

#include <entt/entt.hpp>
#include "Bits.hpp"
#include "FixedString.hpp"
#include "ArenaAllocator.hpp"

#ifndef LayerMask
	#define LayerMask SizedBitField<LAYERS_FIELD_TYPE>
	#define LAYERMASK_ALL = 0xFFFFFFFF
#endif

struct TagComponent
{
	TagComponent(const std::string& string) :
		tag{ string } {}

	FixedString tag;
};

struct LayerComponent
{
	LayerMask layerMask{1};

	operator LayerMask& () {
		return layerMask;
	}
};

enum class BaseLayers : uint32_t
{
	None = 0x00000000,
	Default = 0x00000001
};

struct SceneEntityComponent
{
	entt::entity entity;
	uint16_t sceneIndex;

	bool operator==(const SceneEntityComponent& rhs) const {
		return entity == rhs.entity && sceneIndex == rhs.sceneIndex;
	}
};
struct SceneEntityComponentKeyHash
{
	size_t operator()(const SceneEntityComponent& key) const {
		size_t seed = 0;
		seed ^= std::hash<uint64_t>{}(static_cast<uint64_t>(key.entity)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(key.sceneIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

struct Layer
{
private:
	entt::entity m_entity;

public:
	~Layer() = default;
	Layer() = delete;
	Layer(entt::entity globalEntity) :
		m_entity{ globalEntity } {}
	Layer(const Layer& other) {
		m_entity = other.m_entity;
	}
	Layer& operator=(const Layer& rhs) {
		if (this == &rhs)
			return *this;
		m_entity = rhs.m_entity;
		return *this;
	}
	Layer(Layer&& other) noexcept {
		m_entity = other.m_entity;

		other.m_entity = entt::null;
	}
	Layer& operator=(Layer&& other) noexcept {
		m_entity = other.m_entity;

		other.m_entity = entt::null;
	}
	INLINE bool operator==(const Layer& rhs) {
		return m_entity == rhs.m_entity;
	}

	operator LayerMask&();
	operator const LayerMask&() const;
};

struct BoundingVolume
{
private:

public:
};

#endif