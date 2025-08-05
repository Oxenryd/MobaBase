#include "GameObjectSystem.hpp"
//#include "Scene.h"
#include "Engine.h"

void GameObjectSystem::_emplaceBaseSystems(GameObject& gObject, std::string& name) {
	auto* scene = Engine::getInstance()->getScene(m_sceneIndex);
	assert(scene != nullptr && "Engine::getInstance()->getScene() is NULL!");

	auto& reg = scene->registry();
	
	reg.emplace<EnabledTag>(gObject);
	scene->transformSystem().registryEmplace(gObject, nullptr, nullptr);
	Engine::getInstance()->getGlobalSystem().registerGameObject(gObject, name, m_sceneIndex);
	//scene->nameTagSystem().registryEmplace(entity, reinterpret_cast<void*>(&name), nullptr);
}
