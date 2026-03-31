#ifndef LIGHT_HPP
#define LIGHT_HPP

//#include "HlslTypes.h"
#include "ArenaAllocator.hpp"
#include <glm/glm.hpp>
//using LightComponent = GPULight;

enum class LightType : uint8_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	ENUM_END
};

constexpr static size_t NUM_OF_LIGHT_TYPES = static_cast<size_t>(LightType::ENUM_END);

//enum LightType : uint32_t
//{
//	CastShadow = 1 << 0,
//	// room for more: Static, Volumetric, ContactShadows...
//};

enum class ShadowType : uint32_t
{
	None,
	DirCSM,
	Spot2D,
	PointCube
};

enum class LightFlags : uint32_t
{
	None			= 0x00000000,
	Enabled			= 0x00000001,
	Static			= 0x00000002,
	ShadowCaster	= 0x00000004,
	ShadowFilterPCF = 0x00000008
};

struct alignas (16) LightComponent {
	glm::vec3 position;
	float radius;

	glm::vec3 direction;
	float spotInnerCos;

	glm::vec3 color;
	float intensity;

	alignas(4) LightType type;
	alignas(4) LightFlags flags;
	uint32_t cookieIndex;
	uint32_t shadowIndex;

	float spotOuterCos;
	float falloffExponent;
	float inverseRange;
	float volumetricIntensity;

	float volumetricFalloff;
	float shadowBias;
	float shadowNormalBias;
	uint32_t shadowMatrixIndex;

	ShadowType shadowType;
	uint32_t shadowLayerCount;
	uint32_t _pad0;
	uint32_t _pad1;

	LightComponent() :
	position{ 0.0f, 0.0f, 0.0f},
	radius{1.0f},
	direction{ 0.0f, 0.0f, -1.0f},
	spotInnerCos{1.0f},
	color{ 1.0f, 1.0f, 1.0f},
	intensity{1.0f},
	type{ LightType::Directional },
	flags{ (static_cast<uint32_t>(LightFlags::Enabled) | static_cast<uint32_t>(LightFlags::Static)) },
	cookieIndex{ UINT32_INVALID },
	shadowIndex{ UINT32_INVALID },
	spotOuterCos{ 1.0f },
	falloffExponent{ 1.5f },
	inverseRange{ 1.0f },
	volumetricIntensity{ 1.0f },
	volumetricFalloff{ 1.0f },
	shadowBias{ 0.0f },
	shadowNormalBias{ 0.0f },
	shadowMatrixIndex{ static_cast<uint32_t>(-1) },
	shadowType{ 0 },
	shadowLayerCount{ 0 }
	{}

	// explicit operator const GPULight& () const {
	// 	return *reinterpret_cast<const GPULight*>(this);
	// }
	// explicit operator GPULight& () {
	// 	return *reinterpret_cast<GPULight*>(this);
	// }
};

struct Light
{
private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

public:
	Light(ArenaRegistry* registry, entt::entity entity) :
		m_reg{ registry }, m_entity{ entity } {}

	explicit operator const LightComponent& () const;
};

#endif