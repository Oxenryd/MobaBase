#ifndef SYSTEMECS_H
#define SYSTEMECS_H

#include <entt/entt.hpp>
#include "HlslTypes.h"
#include "ArenaAllocator.hpp"

class SceneBase;
class SystemECS
{
protected:
	uint16_t m_sceneIndex;
	ArenaRegistry* const m_reg;

public:
	SystemECS(ArenaRegistry* const registry, const uint16_t sceneIndex) :
		m_sceneIndex{ sceneIndex },
		m_reg{ registry }
	{}

	virtual entt::entity createEntity() { 
		return m_reg->create();
	};
	virtual void registryEmplace(entt::entity, void*, void**) {
		throw std::runtime_error("SystemECS::registryEmplace() not implemented.");
	}
	virtual void registryRemove(entt::entity, void*) {
		throw std::runtime_error("SystemECS::registryRemove() not implemented.");
	}
	virtual ~SystemECS() = default;
	virtual void run() {}

	SceneBase* getScene();
};

class SystemECS_ModelTransformsProvider : public SystemECS
{
public:
	virtual ~SystemECS_ModelTransformsProvider() = default;
	SystemECS_ModelTransformsProvider(ArenaRegistry* const registry, uint16_t sceneIndex)
		: SystemECS{registry, sceneIndex} {}
	virtual ArenaVector<ModelTransform>& modelTransforms() = 0;
	[[nodiscard]] virtual ArenaVector<ModelTransform>& modelTransforms() const = 0;
};

#endif