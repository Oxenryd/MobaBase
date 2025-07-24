#include "GameObjectSystem.hpp"
#include "Scene.h"
#include "Engine.h"

void GameObjectSystem::_emplaceBaseSystems(entt::entity entity, std::string& name) {
	auto* scene = Engine::getInstance()->getScene(m_sceneIndex);
	assert(scene != nullptr && "Engine::getInstance()->getScene() is NULL!");

	auto& reg = scene->registry();
	
	reg.emplace<EnabledTag>(entity);
	scene->transformSystem().registryEmplace(entity, nullptr, nullptr);
	scene->nameTagSystem().registryEmplace(entity, reinterpret_cast<void*>(&name), nullptr);
}
