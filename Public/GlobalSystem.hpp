#ifndef GLOBALSYSTEMS_HPP
#define GLOBALSYSTEMS_HPP

#include <unordered_map>

#include "GlobalComponents.hpp"
#include "GameObject.hpp"
#include "Hashes.hpp"


class GlobalSystem
{
private:
	entt::registry m_registry;
	std::unordered_map<SceneEntityComponent, entt::entity, SceneEntityComponentKeyHash> m_sceneEntityGlobalMap;

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

		auto sceneEntityComp = m_registry.emplace<SceneEntityComponent>(
			go.m_globalEntity,
			SceneEntityComponent{go.m_entity, sceneIndex});

		m_registry.emplace<TagComponent>(
			go.m_globalEntity,
			TagComponent{tag});

		m_sceneEntityGlobalMap.insert({sceneEntityComp, go.m_globalEntity });
	}

	void printAllTags() {
		Log::logLine(LogType::Info, LogMod::Engine, "GLOBAL SYSTEM, PRINTING ALL TAGS*   (Global), (Scene), (Entity):\n");
		auto view = m_registry.view<TagComponent, SceneEntityComponent>();
		for (auto [entity, tag, sceneEntity] : view.each()) {
			std::cout << "\n\t\t\t\t\t\t\t\t\t\t" << 
				static_cast<uint32_t>(entity) << ",\t" << static_cast<uint32_t>(sceneEntity.sceneIndex) << ",\t" << static_cast<uint32_t>(sceneEntity.entity) << ",\t: "
				<< tag.tag.to_stringView();
		}
		Log::logLine(LogType::Success, LogMod::Engine, "END.");
	}

	std::string_view getTag(SceneEntityComponent sceneEntity) {
		auto it = m_sceneEntityGlobalMap.find(sceneEntity);
		if (it == m_sceneEntityGlobalMap.end()) {
			return std::string_view();
		}
		auto& result = m_registry.get<TagComponent>(it->second);
		return result.tag.to_stringView();
	}
};

#endif