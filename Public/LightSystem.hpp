#ifndef LIGHTSYSTEM_HPP
#define LIGHTSYSTEM_HPP

#include "Scene.h"

#include "Concepts.h"
#include "VulkanContext.hpp"
#include "RenderManager.h"
#include "Engine.h"
#include "Light.hpp"
#include "GameObjectSystem.hpp"



class LightSystem
{
private:
	uint16_t m_sceneIndex = { UINT16_INVALID };
	ArenaRegistry* m_reg;


public:
	LightSystem(ArenaRegistry* registry, const uint16_t sceneIndex) :
		m_sceneIndex{ sceneIndex }, m_reg{ registry } {}

	template<GO_Derived T, typename... LightInfo>
	LightComponent& createNewLight(T* parent, const LightInfo&&... args) {

		auto newLight = LightComponent{std::forward<LightInfo>(args)...};
		std::string name{};
		switch (newLight.type) {
			default:
			case LightType::Directional: {
				name = "DirectionalLight";
			}
		}

		const entt::entity entity = parent
			? parent->entity()
			: Engine::getInstance()->getScene(m_sceneIndex)->gameObjectSystem().createGameObject(name, parent).entity();

		LightComponent& l = m_reg->emplace<LightComponent>(entity, newLight);
		Engine::getInstance()->getRenderManager()->vkContext()->registerNewLight(l);
		return l;
	}
};
#endif