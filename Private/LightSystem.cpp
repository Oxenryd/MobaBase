#include "LightSystem.hpp"
#include "Engine.h"
#include "RenderManager.h"
#include "VulkanContext.hpp"




LightComponent& LightSystem::registerLight(const LightComponent& light, entt::entity entity) {
	auto transformPtr = m_reg->try_get<TransformComponent>(entity);
	if (!transformPtr) {
		//throw std::exception("Light must be attached to entity with transform!");
		m_reg->emplace<TransformComponent>(entity, TransformComponent{});
	}
	auto& thisLight = m_reg->emplace<LightComponent>(entity, light);

	Engine::getInstance()->getRenderManager()->vkContext()->registerNewLight(light);

	return thisLight;
}

