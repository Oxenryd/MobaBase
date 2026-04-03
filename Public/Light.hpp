#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "HlslTypes.h"
#include "ArenaAllocator.hpp"
#include <glm/glm.hpp>

enum class LightType : uint32_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	ENUM_END
};

// static operator LightType(const uint32_t asType) {
// 	return static_cast<LightType>(asType);
// }
//
// static operator uint32_t(const LightType asUint32) {
// 	return static_cast<uint32_t>(asUint32);
// }
//
// static LightType operator=(const uint32_t asType) {
// 	return static_cast<LightType>(asType);
// }
//
// static uint32_t operator=(const LightType asUint32) {
// 	return static_cast<uint32_t>(asUint32);
// }

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

INLINE auto operator| (LightFlags a, LightFlags b) {
	return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}

struct alignas(16) GPULight
{
	// 0..15
	union
	{
		float position_radius[4];
		struct
		{
			FArray3 position;
			float radius;
		};
	};


	// 16..31
	union
	{
		float direction_spotInnerCos[4];
		struct
		{
			FArray3 direction;
			float spotInnerCos;
		};
	};


	// 32..47
	union
	{
		float color_intensity[4];
		struct
		{
			FArray3 color;
			float intensity;
		};
	};


	// 48..63
	LightType type;
	LightFlags flags;
	uint32_t cookieIndex;
	uint32_t shadowIndex;

	// 64..79
	float spotOuterCos;
	float falloffExp;
	float invRange;
	float volumetricIntensity;

	// 80..95
	float volumetricFalloff;
	float shadowBias;
	float shadowNormalBias;
	uint32_t shadowMatrixIndex;

	// 96..111
	uint32_t shadowType;
	uint32_t shadowLayerCount;
	uint32_t _reserved1;
	uint32_t _reserved2;

	GPULight() :
		position_radius{ 0.0f, 0.0f, 0.0f, 1.0f },
		direction_spotInnerCos{ 0.0f, 0.0f, -1.0f, 1.0f },
		color_intensity{ 1.0f, 1.0f, 1.0f, 1.0f },
		type{ static_cast<uint32_t>(LightType::Directional) },
		flags{ (LightFlags::Enabled | LightFlags::Static)},
		cookieIndex{ UINT32_INVALID },
		shadowIndex{ UINT32_INVALID },
		spotOuterCos{ 1.0f },
		falloffExp{ 1.5f },
		invRange{ 1.0f },
		volumetricIntensity{ 1.0f },
		volumetricFalloff{ 1.0f },
		shadowBias{ 0.0f },
		shadowNormalBias{ 0.0f },
		shadowMatrixIndex{ static_cast<uint32_t>(-1) },
		shadowType{ 0 },
		shadowLayerCount{ 0 },
		_reserved1{ 0 },
		_reserved2{ 0 }
	{}
};

typedef GPULight LightComponent;

static_assert(sizeof(GPULight) == 112, "GPULight must be 112 bytes");

// struct alignas (16) LightComponent {
// 	glm::vec3 position;
// 	float radius;
//
// 	glm::vec3 direction;
// 	float spotInnerCos;
//
// 	glm::vec3 color;
// 	float intensity;
//
// 	alignas(4) LightType type;
// 	alignas(4) LightFlags flags;
// 	uint32_t cookieIndex;
// 	uint32_t shadowIndex;
//
// 	float spotOuterCos;
// 	float falloffExponent;
// 	float inverseRange;
// 	float volumetricIntensity;
//
// 	float volumetricFalloff;
// 	float shadowBias;
// 	float shadowNormalBias;
// 	uint32_t shadowMatrixIndex;
//
// 	ShadowType shadowType;
// 	uint32_t shadowLayerCount;
// 	uint32_t _pad0;
// 	uint32_t _pad1;
//
// 	LightComponent() :
// 	position{ 0.0f, 0.0f, 0.0f},
// 	radius{1.0f},
// 	direction{ 0.0f, 0.0f, -1.0f},
// 	spotInnerCos{1.0f},
// 	color{ 1.0f, 1.0f, 1.0f},
// 	intensity{1.0f},
// 	type{ LightType::Directional },
// 	flags{ (static_cast<uint32_t>(LightFlags::Enabled) | static_cast<uint32_t>(LightFlags::Static)) },
// 	cookieIndex{ UINT32_INVALID },
// 	shadowIndex{ UINT32_INVALID },
// 	spotOuterCos{ 1.0f },
// 	falloffExponent{ 1.5f },
// 	inverseRange{ 1.0f },
// 	volumetricIntensity{ 1.0f },
// 	volumetricFalloff{ 1.0f },
// 	shadowBias{ 0.0f },
// 	shadowNormalBias{ 0.0f },
// 	shadowMatrixIndex{ static_cast<uint32_t>(-1) },
// 	shadowType{ 0 },
// 	shadowLayerCount{ 0 }
// 	{}
//
// 	// explicit operator const GPULight& () const {
// 	// 	return *reinterpret_cast<const GPULight*>(this);
// 	// }
// 	// explicit operator GPULight& () {
// 	// 	return *reinterpret_cast<GPULight*>(this);
// 	// }
// };

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

namespace LightFactory
{
	LIGHT_CONST LightComponent Point(
		const glm::vec3& pos,
		const float radius,
		const glm::vec3& color = { 1.0f, 1.0f, 1.0f },
		const float intensity = 1.0f)
	{
		LightComponent L{};
		L.type = LightType::Point;
		L.position = pos;
		L.radius = radius;
		L.invRange = radius > 0.0f ? 1.0f / radius : 0.0f;
		L.color = color;
		L.intensity = intensity;
		return L;
	}

	LIGHT_CONST LightComponent Spot(
		const glm::vec3& pos,
		const glm::vec3& dir,
		const float radius,
		const float innerDeg,
		const float outerDeg,
		const glm::vec3& color = { 1.0f, 1.0f, 1.0f },
		const float intensity = 1.0f)
	{
		LightComponent L{};
		L.type = LightType::Spot;
		L.position = pos;
		auto normDir = glm::normalize(dir);
		L.direction = normDir;
		L.radius = radius;
		L.invRange = radius > 0.0f ? 1.0f / radius : 0.0f;
		L.spotInnerCos = glm::cos(glm::radians(innerDeg));
		L.spotOuterCos = glm::cos(glm::radians(outerDeg));
		L.color = color;
		L.intensity = intensity;
		return L;
	}

	LIGHT_CONST LightComponent Directional(
		const glm::vec3& dir,
		const glm::vec3& color = { 1.0f, 1.0f, 1.0f },
		const float intensity = 1.0f,
		const glm::vec3& position = {0.0f, 50.0f, 0.0f})
	{
		LightComponent L{};
		L.type = LightType::Directional;
		const glm::vec3 normDir = glm::normalize(dir);
		L.direction = normDir;
		L.position = position;
		L.radius = 0.0f;       // Infinite range
		L.invRange = 0.0f;
		L.color = color;
		L.intensity = intensity;
		return L;
	}
}

#endif