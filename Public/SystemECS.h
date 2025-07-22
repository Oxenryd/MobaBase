#ifndef SYSTEMECS_H
#define SYSTEMECS_H

#include <entt/entt.hpp>
#include "HlslTypes.h"
#include "ArenaAllocator.hpp"

class SceneBase;
class SystemECS
{
protected:
	uint32_t m_sceneIndex;
	ArenaRegistry* const m_reg;

public:
	SystemECS(ArenaRegistry* const registry)
		: m_reg{registry} {}
	virtual entt::entity createEntity() { 
		return m_reg->create();
	};
	virtual void registryEmplace(entt::entity, void* valuePtr, void* valueOut) {
		throw std::exception("SystemECS::registryEmplace() not implemented.");
	}
	virtual void registryRemove(entt::entity, void* valueOut) {
		throw std::exception("SystemECS::registryRemove() not implemented.");
	}
	virtual ~SystemECS() = default;
	virtual void run() {};

	SceneBase* getScene();
};

class SystemECS_ModelTransformsProvider : public SystemECS
{
public:
	virtual ~SystemECS_ModelTransformsProvider() = default;
	SystemECS_ModelTransformsProvider(ArenaRegistry* const registry)
		: SystemECS{registry} {}
	virtual std::vector<ModelTransform>& modelTransforms() = 0;
	virtual std::vector<ModelTransform>& modelTransforms() const = 0;
};

#endif