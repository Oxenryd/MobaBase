#ifndef TRANSFORM_SYSTEM_HPP
#define TRANSFORM_SYSTEM_HPP

#include <queue>

#include "SystemECS.h"
#include "ErrorCodes.hpp"
#include "Transform.hpp"
#include "EnabledTag.hpp"

#include "MobaMath.hpp"

class TransformSystem : public SystemECS_ModelTransformsProvider
{
private:
	std::vector<ModelTransform> m_modelTransforms;

public:
	virtual ~TransformSystem() {}
	TransformSystem(entt::registry* const registry)
		: SystemECS_ModelTransformsProvider{registry} {}
	void run() override {

		auto view = m_reg->view<EnabledTag, TransformComponent>();
		for (auto [entity, enabled, transform] : view.each()) {

			if (transform.state.isSet(ObjectState::DirtyTransform)) {
				m_modelTransforms[transform.matrixIndex] = transform.trs();
				transform.state.clearByEnum(ObjectState::DirtyTransform);
			}
		}
	}
	void registryEmplace(entt::entity entity, void* valueInPtr, void* valueOutPtr) override {
		//auto& transform = deref<TransformComponent>(
		//	valueInPtr, "TransformSystem::registryEmplace() called with nullptr. Expected valid TransformComponent*.");
		auto& transComp = m_reg->emplace_or_replace<TransformComponent>(entity, TransformComponent{});
		
		auto matrixIndex = static_cast<uint32_t>(m_modelTransforms.size());
		transComp.matrixIndex = matrixIndex;
		m_modelTransforms.push_back(transComp.trs());
	}

	std::vector<ModelTransform>& modelTransforms() override { 
		return m_modelTransforms;
	}
	std::vector<ModelTransform>& modelTransforms() const override {
		return const_cast<std::vector<ModelTransform>&>(m_modelTransforms);
	}
};

#endif