#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "HlslTypes.h"
#include "ArenaAllocator.hpp"

using LightComponent = GPULight;

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