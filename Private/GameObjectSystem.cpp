#include "GameObjectSystem.hpp"
//#include "Scene.h"
#include "Engine.h"

void GameObjectSystem::_emplaceBaseSystems(GameObject& gObject, const std::string& name, const void* parent) {
	auto* scene = Engine::getInstance()->getScene(m_sceneIndex);
	assert(scene != nullptr && "Engine::getInstance()->getScene() is NULL!");

	auto& reg = scene->registry();
	
	reg.emplace<EnabledTag>(gObject);

	entt::entity* parentEntityPtr = nullptr;
	if (parent) {
		const auto parentGO = static_cast<const GameObject*>(parent);
		parentEntityPtr = const_cast<entt::entity*>(&parentGO->entity());
	}

	scene->transformSystem().registryEmplace(
		gObject,
		parentEntityPtr,
		nullptr);


	Engine::getInstance()->getGlobalSystem().registerGameObject(gObject, name, m_sceneIndex);
	//scene->nameTagSystem().registryEmplace(entity, reinterpret_cast<void*>(&name), nullptr);
}
