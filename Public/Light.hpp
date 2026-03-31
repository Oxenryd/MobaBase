#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "HlslTypes.h"
#include "ArenaAllocator.hpp"

//using LightComponent = GPULight;


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
	flags{ (LightFlags::Enabled | LightFlags::Static)},
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

	operator Light& () = delete;
	operator LightComponent& ();
};

#endif