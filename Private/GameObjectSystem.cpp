#include "GameObjectSystem.hpp"
#include "Scene.h"
#include "Engine.h"
#include "SceneTrackerSystem.h"

void GameObjectSystem::_emplaceBaseSystems(entt::entity entity, std::string& name) {
	auto* scene = Engine::getInstance()->getScene(m_sceneIndex);
	assert(scene != nullptr && "Engine::getInstance()->getScene() is NULL!");

	auto& reg = scene->registry();
	
	scene->enabledSystem().registryEmplace(entity, nullptr, nullptr);
	scene->transformSystem().registryEmplace(entity, nullptr, nullptr);
	reg.emplace_or_replace<SceneTrackerComponent>(entity);
	scene->nameTagSystem().registryEmplace(entity, reinterpret_cast<void*>(&name), nullptr);
}
