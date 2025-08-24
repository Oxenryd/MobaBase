#ifndef TRANSFORM_SYSTEM_HPP
#define TRANSFORM_SYSTEM_HPP

#include <thread>
#include <semaphore>
#include <array>
#include <atomic>
#include <algorithm>

#include "SystemECS.h"
#include "ErrorCodes.hpp"
#include "Transform.hpp"
#include "EnabledTag.hpp"
#include "MobaMath.hpp"
#include "BoundingSystem.h"
#include "Range.hpp"



class TransformSystem : public SystemECS_ModelTransformsProvider
{
	struct StackItem { entt::entity e; bool parentDirty; };
private:
	std::array<std::vector<StackItem>, TRANSFORM_THREADS_MAX> m_threadsStacks;
	ArenaUMap<entt::entity, size_t> m_entityRootIndexMap;
	ArenaVector<entt::entity> m_roots;
	ArenaVector<ModelTransform> m_modelTransforms;
	ArenaVector<entt::entity> m_parentOf;
	ArenaVector<std::vector<entt::entity>> m_childrenOf;

	std::array<std::thread*, TRANSFORM_THREADS_MAX> m_workers;
	std::array<Range, TRANSFORM_THREADS_MAX> m_workerRanges;
	uint8_t m_curNumWorkers = 1;
	BoundingSystem* m_boundSysPtr = nullptr;
	std::array<std::binary_semaphore*, TRANSFORM_THREADS_MAX> m_startSemas;
	std::array<std::binary_semaphore*, TRANSFORM_THREADS_MAX> m_doneSemas;
	bool m_threadsRunning = true;
	

	static void _workerThread(TransformSystem* this_, uint8_t tI) {
		while (true) {
			
			this_->m_startSemas[tI]->acquire();

			if (!this_->m_threadsRunning)
				return;


			// Push roots (cache this list, or gather each frame if you prefer)
			auto& stack = this_->m_threadsStacks[tI];
			Range& range = this_->m_workerRanges[tI];
			for (size_t i = range.start(); i < range.end(); ++i) {
				auto& root = this_->m_roots[i];
				stack.push_back({ root, false });
			}

			while (!stack.empty()) {
				const auto [e, parentDirty] = stack.back();
				stack.pop_back();

				if (!this_->m_reg->all_of<EnabledTag>(e))
					continue;

				auto& t = this_->m_reg->get<TransformComponent>(e);
				t.state.clearByEnum(ObjectState::MovedThisFrame);
				const bool selfDirtyLocal = t.state.hasFlag(ObjectState::DirtyTransform);
				const bool worldDirty = parentDirty || selfDirtyLocal;

				if (worldDirty) {
					// Compute local TRS
					glm::mat4 local = t.trs();

					// Compose with parent (parent world is guaranteed up-to-date now)
					if (!t.state.hasFlag(ObjectState::IgnoreParentTransform)) {
						const entt::entity p = this_->m_parentOf.empty() ? entt::null : this_->m_parentOf[entt::to_integral(e)];
						if (p != entt::null /* && m_reg->valid(p) */ && this_->m_reg->all_of<TransformComponent>(p)) {
							const auto& pt = this_->m_reg->get<TransformComponent>(p);
							this_->m_modelTransforms[t.matrixIndex] = this_->m_modelTransforms[pt.matrixIndex] * local;
						} else {
							this_->m_modelTransforms[t.matrixIndex] = local;
						}
					} else {
						this_->m_modelTransforms[t.matrixIndex] = local;
					}

					// Bounds update
					if (auto b = this_->m_reg->try_get<BoundingVolumeComponent>(e)) {
						const auto& localAABB = this_->m_boundSysPtr->cachedLocals()[b->coarseIndexLocal];
						this_->m_boundSysPtr->aabbs()[b->coarseIndexWorld] =
							localAABB.transformed_noPerspective(this_->m_modelTransforms[t.matrixIndex]);
					}

					// Flags
					t.state.clearByEnum(ObjectState::DirtyTransform);
					if (selfDirtyLocal) t.state.setByEnum(ObjectState::MovedThisFrame);
					if (parentDirty) t.state.setByEnum(ObjectState::ParentMovedThisFrame);


					// Children
					const auto& kids = this_->m_childrenOf[entt::to_integral(e)];
					for (auto child : kids) {
						stack.push_back({ child, worldDirty });
					}
				}
			}
			this_->m_doneSemas[tI]->release();
		}
	}

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

	INLINE void _checkEntityIsRoot(entt::entity e) {
		auto id = entt::to_integral(e);
		
		auto parent = m_parentOf[id];
		if (parent == entt::null) {
			auto it = m_entityRootIndexMap.find(e);
			if (it == m_entityRootIndexMap.end()) {
				auto index = m_roots.size();
				m_roots.push_back(e);
				m_entityRootIndexMap.insert({ e, index });
			}

			} else {
				auto it = m_entityRootIndexMap.find(e);
				if (it != m_entityRootIndexMap.end()) {
					m_roots[it->second] = m_roots.back();
					m_roots.pop_back();
				}
		}
	}

public:
	virtual ~TransformSystem() {
		m_reg->on_destroy<TransformComponent>()
			.disconnect<&TransformSystem::_onDestroy>(this);

		m_threadsRunning = false;
		for (size_t i = 0; i < TRANSFORM_THREADS_MAX; ++i) {
			m_startSemas[i]->release();
			m_workers[i]->join();
			delete m_workers[i];
			m_workers[i] = nullptr;
			delete m_startSemas[i];
			m_startSemas[i] = nullptr;
			delete m_doneSemas[i];
			m_doneSemas[i] = nullptr;
		}
	}

	TransformSystem(ArenaRegistry* const registry, uint16_t sceneIndex, Arena* const arena)
		: SystemECS_ModelTransformsProvider{ registry, sceneIndex },
		m_modelTransforms{ ArenaAllocator<ModelTransform>(arena) },
		m_parentOf{ ArenaAllocator<entt::entity>(arena) },
		m_childrenOf{ ArenaAllocator<entt::entity>(arena) },
		m_roots{ ArenaAllocator<entt::entity>(arena) },
		m_entityRootIndexMap{ ArenaAllocator<std::pair<entt::entity, size_t>>{arena} }
{
		registry->on_destroy<TransformComponent>()
			.connect<&TransformSystem::_onDestroy>(this);

		for (auto& stack : m_threadsStacks)
			stack.reserve(256);

		// Start Threads
		for (uint8_t i = 0; i < TRANSFORM_THREADS_MAX; ++i) {
			m_workers[i] = new std::thread{ TransformSystem::_workerThread, this, i };
			m_startSemas[i] = new std::binary_semaphore{ 0 };
			m_doneSemas[i] = new std::binary_semaphore{ 0 };
		}
	}


	INLINE void run(BoundingSystem& boundSys) {

		//TEST MT
		static constexpr size_t workerDiv = (TRANSFORM_THREADS_MAX * (TRANSFORM_THREADS_MAX / 2));
		m_boundSysPtr = &boundSys;
		m_curNumWorkers = static_cast<uint8_t>(
			std::clamp(m_roots.size() / workerDiv,
					   static_cast <size_t>(1),
					   static_cast<size_t>(TRANSFORM_THREADS_MAX)));

		uint32_t offset = static_cast<uint32_t>(
				std::ceil(static_cast<float>(m_roots.size() / static_cast<float>(m_curNumWorkers)))
			);
		for (uint32_t i = 0; i < m_curNumWorkers; ++i) {
			uint32_t start = i * offset;
			uint32_t count = (i == m_curNumWorkers - 1 && m_roots.size() % offset != 0)
				? m_roots.size() % offset
				: offset;
			m_workerRanges[i] = { start, count };
			m_startSemas[i]->release();
		}

		for (size_t i = 0; i < m_curNumWorkers; ++i) {
			m_doneSemas[i]->acquire();
		}
	}




	INLINE void registryEmplace(entt::entity entity, void* valueInPtr = nullptr, void** valueOutPtr = nullptr) override {
		auto& transComp = m_reg->emplace_or_replace<TransformComponent>(entity, TransformComponent{});
		
		auto matrixIndex = static_cast<uint32_t>(m_modelTransforms.size());
		transComp.matrixIndex = matrixIndex;
		transComp.sceneIndex = m_sceneIndex;
		auto index = entt::to_integral(entity);
		if (index >= m_parentOf.size())
			m_parentOf.resize(index + 1, entt::null);
		if (index >= m_childrenOf.size())
			m_childrenOf.resize(index + 1);
		m_modelTransforms.push_back(transComp.trs());

		if (valueInPtr) {
			auto parent = static_cast<entt::entity*>(valueInPtr);
			setParent(entity, parent);
		}
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
			_checkEntityIsRoot(oldParent);
		}

		if (parent) {
			entt::entity newParent = *parent;
			if (newParent == entt::null) {
				m_parentOf[id] = entt::null;
				auto it = m_entityRootIndexMap.find(entity);
				if (it != m_entityRootIndexMap.end()) {
					m_roots[it->second] = m_roots.back();
					m_roots.pop_back();
				}
				return;
			}

			auto newId = entt::to_integral(newParent);
			if (newId >= m_childrenOf.size()) {
				m_childrenOf.resize(newId + 1);
			}

			m_parentOf[id] = newParent;
			m_childrenOf[newId].push_back(entity);
			_checkEntityIsRoot(newParent);

		} else {
			m_parentOf[id] = entt::null;
			auto it = m_entityRootIndexMap.find(entity);
			if (it != m_entityRootIndexMap.end()) {
				m_roots[it->second] = m_roots.back();
				m_roots.pop_back();
			}
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


	INLINE void printHierarchy() {
		Log::logLine(LogType::Info, LogMod::Engine,
					 std::format("\nTransform Hierarchy for Scene index {} - *\n", m_sceneIndex) );
		
		uint32_t depth = 0;
		for (auto root : m_roots) {			
			logTransformTag(root, 0);
		}
		Log::logLine(LogType::Success, LogMod::Engine, "Done.");
	}

	INLINE void logTransformTag(entt::entity entity, uint32_t depth) {
		Transform transform{ m_reg, entity };
		auto tag = transform.getTag();
		std::string tagStr = tag.empty() ? "untagged" : std::string{ tag };
		std::string tab = "\t";
		for (size_t i = 0; i < depth; ++i) {
			if (i == depth - 1)
				tab.append("  L ");
			else
				tab.append("\t");
		}
		std::cout << tab << static_cast<uint32_t>(entity) << ", " << tagStr << "\n";
		//LOGLINE_IND(LogType::Info, LogMod::Engine, tagStr, depth);
		for (auto child : m_childrenOf[entt::to_integral(entity)])
			logTransformTag(child, depth + 1);
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