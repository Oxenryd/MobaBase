#ifndef GLOBALSYSTEMS_HPP
#define GLOBALSYSTEMS_HPP

#include "GlobalComponents.hpp"
#include "GameObject.hpp"


class GlobalSystem
{
private:
	entt::registry m_registry;

public:
	~GlobalSystem() {}
	GlobalSystem(const GlobalSystem&) = delete;
	GlobalSystem& operator=(const GlobalSystem&) = delete;
	GlobalSystem(GlobalSystem&&) = delete;
	GlobalSystem& operator=(GlobalSystem&&) = delete;
	GlobalSystem() {}

	entt::registry& registry() { return m_registry; }

	void registerGameObject(GameObject& go, const std::string& tag, uint16_t sceneIndex) {
		go.m_globalEntity = m_registry.create();

		m_registry.emplace<LayerComponent>(
			go.m_globalEntity,
			LayerComponent{});

		m_registry.emplace<SceneEntityComponent>(
			go.m_globalEntity,
			SceneEntityComponent{go.m_entity, sceneIndex});

		m_registry.emplace<TagComponent>(
			go.m_globalEntity,
			TagComponent{tag});
	}
};

#endif