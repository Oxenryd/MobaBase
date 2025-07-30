#ifndef TRANSFORM_SYSTEM_HPP
#define TRANSFORM_SYSTEM_HPP

#include "SystemECS.h"
#include "ErrorCodes.hpp"
#include "Transform.hpp"
#include "EnabledTag.hpp"
#include "MobaMath.hpp"



class TransformSystem : public SystemECS_ModelTransformsProvider
{
private:
	ArenaVector<ModelTransform> m_modelTransforms;
	ArenaVector<entt::entity> m_parentOf;
	ArenaVector<std::vector<entt::entity>> m_childrenOf;

	INLINE void _onDestroy(ArenaRegistry& reg, entt::entity entity) {
		auto id = entt::to_integral(entity);
		if (id < m_parentOf.size()) {
			entt::entity parent = m_parentOf[id];
			if (parent != entt::null) {
				auto& siblings = m_childrenOf[entt::to_integral(parent)];
				std::erase(siblings, entity);
			}

			m_parentOf[id] = entt::null;
			m_childrenOf[id].clear(); // optionally recursively destroy?
		}
	}


public:
	virtual ~TransformSystem() {
		m_reg->on_destroy<TransformComponent>()
			.disconnect<&TransformSystem::_onDestroy>(this);
	}
	TransformSystem(ArenaRegistry* const registry, uint16_t sceneIndex, Arena* const arena)
		: SystemECS_ModelTransformsProvider{registry, sceneIndex},
		m_modelTransforms{ ArenaAllocator<ModelTransform>(arena) },
		m_parentOf{ ArenaAllocator<entt::entity>(arena) },
		m_childrenOf{ ArenaAllocator<entt::entity>(arena) }
	{
		registry->on_destroy<TransformComponent>()
			.connect<&TransformSystem::_onDestroy>(this);
	}

	INLINE void run() override {

		auto view = m_reg->view<EnabledTag, TransformComponent>();

		for (auto entity : view) {
			auto& transform = view.get<TransformComponent>(entity);
			auto id = entt::to_integral(entity);

			if (!transform.state.isSet(ObjectState::DirtyTransform))
				continue;

			// Build local matrix
			glm::mat4 local = transform.trs();

			// Combine with parent
			if (!(transform.state.isSet(ObjectState::IgnoreParentTransform))) {
				entt::entity parent = m_parentOf.empty() ? entt::null : m_parentOf[id];
				if (parent != entt::null && m_reg->valid(parent) && m_reg->all_of<TransformComponent>(parent)) {
					const auto& parentTransform = m_reg->get<TransformComponent>(parent);
					m_modelTransforms[transform.matrixIndex] = 
						m_modelTransforms[parentTransform.matrixIndex] * local;
				} else {
					m_modelTransforms[transform.matrixIndex] = local;
				}
			} else {
				m_modelTransforms[transform.matrixIndex] = local;
			}

			transform.state.clearByEnum(ObjectState::DirtyTransform);
		}



	}

	INLINE void registryEmplace(entt::entity entity, void* valueInPtr, void* valueOutPtr) override {
		auto& transComp = m_reg->emplace_or_replace<TransformComponent>(entity, TransformComponent{});
		
		auto matrixIndex = static_cast<uint32_t>(m_modelTransforms.size());
		transComp.matrixIndex = matrixIndex;
		transComp.sceneIndex = m_sceneIndex;
		m_modelTransforms.push_back(transComp.trs());
	}

	INLINE ArenaVector<ModelTransform>& modelTransforms() override {
		return m_modelTransforms;
	}
	INLINE ArenaVector<ModelTransform>& modelTransforms() const override {
		return const_cast<ArenaVector<ModelTransform>&>(m_modelTransforms);
	}

	INLINE void setParent(const entt::entity entity, const entt::entity* parent) {
		auto id = entt::to_integral(entity);

		if (id >= m_parentOf.size()) {
			m_parentOf.resize(id + 1, entt::null);
			m_childrenOf.resize(id + 1);
		}

		entt::entity oldParent = m_parentOf[id];
		if (oldParent != entt::null) {
			auto& childrenToOldParent = m_childrenOf[entt::to_integral(oldParent)];
			std::erase(childrenToOldParent, entity);
		}

		if (parent) {
			entt::entity newParent = *parent;

			auto newId = entt::to_integral(newParent);
			if (newId >= m_childrenOf.size()) {
				m_childrenOf.resize(newId + 1);
			}

			m_parentOf[id] = newParent;
			m_childrenOf[newId].push_back(entity);
		} else {
			m_parentOf[id] = entt::null;
		}
	}

	INLINE std::span<entt::entity> getChildren(const entt::entity ofEntity) {
		auto id = entt::to_integral(ofEntity);
		if (id >= m_childrenOf.size())
			return std::span<entt::entity>();

		return std::span<entt::entity>(m_childrenOf[id].data(), m_childrenOf[id].size());
	}

	INLINE entt::entity getParent(const entt::entity ofEntity) {
		auto id = entt::to_integral(ofEntity);
		if (id >= m_parentOf.size())
			return entt::null;
		
		return m_parentOf[id];
	}

	INLINE void clearChildren(const entt::entity parent) {
		auto parentId = entt::to_integral(parent);
		if (parentId >= m_childrenOf.size()) return;

		for (auto child : m_childrenOf[parentId]) {
			m_parentOf[entt::to_integral(child)] = entt::null;
		}
		m_childrenOf[parentId].clear();
	}
};


namespace entt
{
	template<>
	struct storage_type<TransformComponent, ArenaRegistry>
	{
		using type = ArenaStorage<TransformComponent>;
	};
}


#endif