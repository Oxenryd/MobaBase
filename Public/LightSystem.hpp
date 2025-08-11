#ifndef LIGHTSYSTEM_HPP
#define LIGHTSYSTEM_HPP

#include <vector>
#include <entt/entt.hpp>

#include "GameObject.hpp"
#include "Transform.hpp"
#include "Light.hpp"

class LightSystem
{
private:
	uint16_t m_sceneIndex = { UINT16_INVALID };
	ArenaRegistry* m_reg;


public:
	LightSystem(ArenaRegistry* registry, uint16_t sceneIndex) :
		m_sceneIndex{ sceneIndex }, m_reg{ registry } {}

	LightComponent& registerLight(const LightComponent& light, entt::entity entity);
};
#endif