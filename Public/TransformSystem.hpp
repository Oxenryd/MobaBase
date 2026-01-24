#ifndef TRANSFORM_SYSTEM_HPP
#define TRANSFORM_SYSTEM_HPP

#include <thread>
#include <semaphore>
#include <array>
#include <atomic>
#include <algorithm>

#include "SystemECS.h"
#include "Transform.hpp"
#include "EnabledTag.hpp"
#include "MMath.hpp"
#include "BoundingSystem.h"
#include "Range.hpp"
#include "Profiler.hpp"

using TransformGroup =
decltype(std::declval<ArenaRegistry&>()
		 .group<TransformComponent, BoundingVolumeComponent, EnabledTag>());

class TransformSystem final : public SystemECS_ModelTransformsProvider
{
	struct StackItem { entt::entity e; ObjectStateType dirtyState; };
	std::array<std::vector<StackItem>, TRANSFORM_THREADS_MAX> m_threadsStacks;
	ArenaUMap<entt::entity, size_t> m_entityRootIndexMap;
	ArenaVector<entt::entity> m_roots;
	ArenaVector<ModelTransform> m_modelTransforms;
	ArenaVector<entt::entity> m_parentOf;
	ArenaVector<std::vector<entt::entity>> m_childrenOf;

	// Temps
	TransformGroup* m_groupPtr = nullptr;

	// RenderSync
	//bool m_systemDirty = false;

	// Component Data
	ArenaVector<glm::vec3> m_positions;
	ArenaVector<glm::quat> m_rotations;
	ArenaVector<glm::vec3> m_scales;

	std::array<std::thread*, TRANSFORM_THREADS_MAX> m_workers;
	std::array<Range<uint32_t>, TRANSFORM_THREADS_MAX> m_workerRanges;
	std::array<std::vector<entt::entity>, TRANSFORM_THREADS_MAX> m_workerEntities;
	std::atomic<uint8_t> m_curNumWorkers = 0;
	BoundingSystem* m_boundSysPtr = nullptr;
	std::array<std::binary_semaphore*, TRANSFORM_THREADS_MAX> m_startSemas;
	std::array<std::binary_semaphore*, TRANSFORM_THREADS_MAX> m_doneSemas;
	bool m_threadsRunning = true;
	


	// Optimized batch update function
	static void _brute_update_transform_batch_avx2(
		const std::vector<entt::entity>& entities,
		TransformSystem* const this_,
		const size_t start_idx,
		const size_t count) {

		PROFILE_SCOPE("TransformWorkerAVX");

		constexpr size_t batch_size = 4;
		const size_t batched_count = count / batch_size * batch_size;

		// Process in batches of 4
		for (size_t i = start_idx; i < start_idx + batched_count; i += batch_size) {
			glm::vec3 positions[4];
			glm::quat rotations[4];
			glm::vec3 scales[4];
			glm::mat4 local_matrices[4];

			// Gather data for 4 entities
			for (int j = 0; j < 4; ++j) {
				const entt::entity e = entities[i + j];
				//if (!this_->m_reg->all_of<EnabledTag>(e)) continue;

				const auto& t = this_->m_groupPtr->get<TransformComponent>(e);
				positions[j] = this_->m_positions[t.dataIndex];
				rotations[j] = this_->m_rotations[t.dataIndex];
				scales[j] = this_->m_scales[t.dataIndex];
			}

			// Build 4 TRS matrices with SIMD
			MMath::build_trs_matrices_avx2(positions, rotations, scales, local_matrices);

			// Apply parent transforms and store results
			for (int j = 0; j < 4; ++j) {
				const entt::entity e = entities[i + j];
				//if (!this_->m_reg->all_of<EnabledTag>(e)) continue;

				auto& t1 = this_->m_groupPtr->get<TransformComponent>(e);
				const entt::entity p = this_->m_parentOf[entt::to_integral(e)];

				if (p != entt::null) {
					const auto& pt = this_->m_groupPtr->get<TransformComponent>(p);
					if (objectState_is_dirty(pt.state.value())) t1.state.setByEnum(ObjectState::ParentMovedThisFrame);
					auto& mtw = this_->m_modelTransforms[t1.dataIndex].modelToWorld;
					this_->m_modelTransforms[t1.dataIndex] = MMath::matrix_multiply_avx2(
						mtw, local_matrices[j]);
				} else {
					this_->m_modelTransforms[t1.dataIndex] = local_matrices[j];
				}

				// Always update bounds
				//if (auto b = this_->m_reg->try_get<BoundingVolumeComponent>(e)) {
				//	const auto& localAABB = this_->m_boundSysPtr->cachedLocals()[b->coarseIndexLocal];
				//	this_->m_boundSysPtr->aabbs()[b->coarseIndexWorld] =
				//		localAABB.transformed_noPerspective(this_->m_modelTransforms[t.dataIndex]);
				//}

				if (objectState_is_dirty(t1.state.value())) t1.state.setByEnum(ObjectState::MovedThisFrame);

				// Flags
				t1.state.clearByEnum(
					static_cast<ObjectState>(static_cast<ObjectStateType>(ObjectState::DirtyTransform) |
					                         static_cast<ObjectStateType>(ObjectState::TranslationDirty) |
					                         static_cast<ObjectStateType>(ObjectState::RotationDirty) |
					                         static_cast<ObjectStateType>(ObjectState::ScaleDirty))
				);

				// Children
				const auto& kids = this_->m_childrenOf[entt::to_integral(e)];
				if (kids.size() >= 4) {
					auto tRange = this_->_setupWorkerThreadRange(kids.data(), kids.size());
					if (tRange.count > 0) {
						for (size_t t2 = tRange.start(); t2 < tRange.end(); ++t2) {
							this_->m_startSemas[t2]->release();
						}
						for (size_t t2 = tRange.start(); t2 < tRange.end(); ++t2) {
							this_->m_doneSemas[t2]->acquire();
						}
					} else {
						_brute_update_transform_batch_avx2(kids, this_, 0, kids.size());
					}
					
				} else {
					for (const auto child : kids) {
						_brute_update_transform_scalar(child, this_);
					}
				}
			}
		}

		// Handle remaining entities with scalar code
		for (size_t i = start_idx + batched_count; i < start_idx + count; ++i) {
			_brute_update_transform_scalar(entities[i], this_);
		}
	}

	static void _brute_update_transform_scalar(const entt::entity root, TransformSystem* const this_) {
		//if (!this_->m_reg->all_of<EnabledTag>(root)) return;

		PROFILE_SCOPE("TransformWorkerScalar");

		auto& t = this_->m_groupPtr->get<TransformComponent>(root);
		const glm::mat4 local =
			MMath::composeTRS(
				this_->m_positions[t.dataIndex],
				this_->m_rotations[t.dataIndex], this_->m_scales[t.dataIndex]);

		t.state.clearByEnum(ObjectState::MovedThisFrame);

		const auto intEntity = entt::to_integral(root);

		// Always update, no dirty checks
		const entt::entity p = this_->m_parentOf[intEntity];
		if (p != entt::null) {
			const auto& pt = this_->m_groupPtr->get<TransformComponent>(p);
			if (objectState_is_dirty(pt.state.value())) t.state.setByEnum(ObjectState::ParentMovedThisFrame);

			this_->m_modelTransforms[t.dataIndex].modelToWorld = MMath::matrix_multiply_avx2(
				this_->m_modelTransforms[pt.dataIndex].modelToWorld, local);
		} else {
			this_->m_modelTransforms[t.dataIndex].modelToWorld = local;
		}

		//// Always update bounds
		//if (auto b = this_->m_reg->try_get<BoundingVolumeComponent>(root)) {
		//	const auto& localAABB = this_->m_boundSysPtr->cachedLocals()[b->coarseIndexLocal];
		//	this_->m_boundSysPtr->aabbs()[b->coarseIndexWorld] =
		//		localAABB.transformed_noPerspective(this_->m_modelTransforms[t.dataIndex]);
		//}

		if (objectState_is_dirty(t.state.value())) t.state.setByEnum(ObjectState::MovedThisFrame);

		// Flags
		t.state.clearByEnum(
			static_cast<ObjectState>(static_cast<ObjectStateType>(ObjectState::DirtyTransform) |
			                         static_cast<ObjectStateType>(ObjectState::TranslationDirty) |
			                         static_cast<ObjectStateType>(ObjectState::RotationDirty) |
			                         static_cast<ObjectStateType>(ObjectState::ScaleDirty))
		);



		const auto& kids = this_->m_childrenOf[intEntity];
		if (kids.size() >= 4) {
			auto tRange = this_->_setupWorkerThreadRange(kids.data(), kids.size());
			if (tRange.count > 0) {
				for (size_t t2 = tRange.start(); t2 < tRange.end(); ++t2) {
					this_->m_startSemas[t2]->release();
				}
				for (size_t t2 = tRange.start(); t2 < tRange.end(); ++t2) {
					this_->m_doneSemas[t2]->acquire();
				}
			} else {
				_brute_update_transform_batch_avx2(kids, this_, 0, kids.size());
			}

		} else {
			for (const auto child : kids) {
				_brute_update_transform_scalar(child, this_);
			}
		}




		//// Children
		//const auto& kids = this_->m_childrenOf[entt::to_integral(root)];
		//if (kids.size() >= 4) {
		//	_brute_update_transform_batch_avx2(kids, this_, 0, kids.size());
		//} else {
		//	for (auto child : kids) {
		//		_brute_update_transform_scalar(child, this_);
		//	}
		//}
	}

	//static void _workerThread_avx2_test(TransformSystem* this_, uint8_t tI) {
	//	while (true) {
	//		this_->m_startSemas[tI]->acquire();
	//		if (!this_->m_threadsRunning) return;

	//		Range<uint32_t>& range = this_->m_workerRanges[tI];
	//		thread_local static std::vector<entt::entity> entities;
	//		entities.clear();

	//		// Collect entities in this range
	//		for (size_t i = range.start(); i < range.end(); ++i) {
	//			entities.push_back(this_->m_roots[i]);
	//		}

	//		// Process in batches of 4 with AVX2
	//		_brute_update_transform_batch_avx2(entities, this_, 0, entities.size());

	//		this_->m_doneSemas[tI]->release();
	//	}
	//}

	static void _workerThread_avx2_test_entities(TransformSystem* const this_, const uint8_t tI) {
		while (true) {
			this_->m_startSemas[tI]->acquire();

			{
				if (!this_->m_threadsRunning) return;

				// Process in batches of 4 with AVX2
				_brute_update_transform_batch_avx2(
					this_->m_workerEntities[tI], this_, 0, this_->m_workerEntities[tI].size());
			}
			this_->m_doneSemas[tI]->release();
		}
	}

	//// Test: Skip ALL dirty checking, just brute force update everything
	//static void _workerThread_brute_test(TransformSystem* this_, uint8_t tI) {
	//	while (true) {
	//		this_->m_startSemas[tI]->acquire();
	//		if (!this_->m_threadsRunning) return;

	//		Range<uint32_t>& range = this_->m_workerRanges[tI];

	//		// Brute force: update every transform in range
	//		for (size_t i = range.start(); i < range.end(); ++i) {
	//			auto& root = this_->m_roots[i];
	//			_brute_update_transform_scalar(root, this_);
	//		}

	//		this_->m_doneSemas[tI]->release();
	//	}
	//}


	//static void _workerThread(TransformSystem* this_, uint8_t tI) {
	//	while (true) {
	//		
	//		this_->m_startSemas[tI]->acquire();

	//		if (!this_->m_threadsRunning)
	//			return;


	//		// Push roots (cache this list, or gather each frame if you prefer)
	//		auto& stack = this_->m_threadsStacks[tI];
	//		Range<uint32_t>& range = this_->m_workerRanges[tI];
	//		for (size_t i = range.start(); i < range.end(); ++i) {
	//			auto& root = this_->m_roots[i];

	//			if (!this_->m_reg->all_of<EnabledTag>(root))
	//				continue;

	//			auto& rootTransform = this_->m_reg->get<TransformComponent>(root);
	//			//if (rootTransform.state.hasFlag(ObjectState::DirtyTransform)) {
	//				stack.push_back({ root, rootTransform.state.value()});
	//			//}
	//		}

	//		while (!stack.empty()) {
	//			const auto [e, rootState] = stack.back();
	//			stack.pop_back();

	//			auto& t = this_->m_reg->get<TransformComponent>(e);
	//			t.state.clearByEnum(ObjectState::MovedThisFrame);
	//			const ObjectStateType selfState = t.state.value();
	//			const ObjectStateType worldDirty = rootState | selfState;

	//			if (!objectState_is_dirty(worldDirty)) {
	//				continue;
	//			}

	//			// Compose with parent
	//			if (!t.state.hasFlag(ObjectState::IgnoreParentTransform) && objectState_is_dirty(rootState) )
	//			{
	//				const entt::entity p = this_->m_parentOf.empty() ? entt::null : this_->m_parentOf[entt::to_integral(e)];
	//				if (p != entt::null && this_->m_reg->all_of<TransformComponent>(p)) {
	//					const auto& pt = this_->m_reg->get<TransformComponent>(p);
	//					const auto& parentState = pt.state.value();
	//					if (
	//						( !objectState_is_dirty(parentState) || objectState_only_translation_dirty(parentState) ) &&
	//						!t.state.hasFlag(ObjectState::RotationDirty) &&
	//						!t.state.hasFlag(ObjectState::ScaleDirty)
	//						)
	//					{
	//						this_->m_modelTransforms[t.dataIndex].modelToWorld[3] = 
	//											  this_->m_modelTransforms[pt.dataIndex].modelToWorld * glm::vec4(this_->m_positions[t.dataIndex], 1.0f);
	//					} else {
	//						glm::mat4 local = MMath::composeTRS(this_->m_positions[t.dataIndex], this_->m_rotations[t.dataIndex], this_->m_scales[t.dataIndex]); //t.trs();
	//						this_->m_modelTransforms[t.dataIndex] =
	//							this_->m_modelTransforms[pt.dataIndex] * local;
	//					}
	//				} else { // Parent == null
	//					if (objectState_only_translation_dirty(selfState)) {
	//						this_->m_modelTransforms[t.dataIndex].modelToWorld[3] = glm::vec4(this_->m_positions[t.dataIndex], 1.0f);
	//					} else {
	//						glm::mat4 local = MMath::composeTRS(this_->m_positions[t.dataIndex], this_->m_rotations[t.dataIndex], this_->m_scales[t.dataIndex]);//t.trs();
	//						this_->m_modelTransforms[t.dataIndex] = local;
	//					}
	//				}
	//			} else if (objectState_is_dirty(selfState)) {
	//				if (objectState_only_translation_dirty(selfState)) {
	//					this_->m_modelTransforms[t.dataIndex].modelToWorld[3] = glm::vec4(this_->m_positions[t.dataIndex], 1.0f);
	//				} else {
	//					glm::mat4 local = MMath::composeTRS(this_->m_positions[t.dataIndex], this_->m_rotations[t.dataIndex], this_->m_scales[t.dataIndex]);//t.trs();
	//					this_->m_modelTransforms[t.dataIndex] = local;
	//				}
	//			}

	//			// Bounds update
	//			if (auto b = this_->m_reg->try_get<BoundingVolumeComponent>(e)) {
	//				const auto& localAABB = this_->m_boundSysPtr->cachedLocals()[b->coarseIndexLocal];
	//				this_->m_boundSysPtr->aabbs()[b->coarseIndexWorld] =
	//					localAABB.transformed_noPerspective(this_->m_modelTransforms[t.dataIndex]);
	//			}

	//			// Flags
	//			t.state.clearByEnum( 
	//				(ObjectState)(
	//					static_cast<ObjectStateType>(ObjectState::DirtyTransform) | 
	//					static_cast<ObjectStateType>(ObjectState::TranslationDirty) |
	//					static_cast<ObjectStateType>(ObjectState::RotationDirty) |
	//					static_cast<ObjectStateType>(ObjectState::ScaleDirty)
	//				)
	//			);

	//			if (objectState_is_dirty(selfState)) t.state.setByEnum(ObjectState::MovedThisFrame);
	//			if (objectState_is_dirty(rootState)) t.state.setByEnum(ObjectState::ParentMovedThisFrame);


	//			// Children
	//			const auto& kids = this_->m_childrenOf[entt::to_integral(e)];
	//			for (auto child : kids) {
	//				stack.push_back({ child, worldDirty });
	//			}

	//		}
	//		this_->m_doneSemas[tI]->release();
	//	}
	//}

	INLINE void _onDestroy(ArenaRegistry&, const entt::entity entity) {
		const auto id = entt::to_integral(entity);
		if (id < m_parentOf.size()) {
			const entt::entity parent = m_parentOf[id];
			if (parent != entt::null) {
				auto& siblings = m_childrenOf[entt::to_integral(parent)];
				std::erase(siblings, entity);
			}

			m_parentOf[id] = entt::null;
			m_childrenOf[id].clear(); // optionally recursively destroy?
		}
	}

	INLINE void _checkEntityIsRoot(entt::entity e) {
		const auto id = entt::to_integral(e);
		
		const auto parent = m_parentOf[id];
		if (parent == entt::null) {
			const auto it = m_entityRootIndexMap.find(e);
			if (it == m_entityRootIndexMap.end()) {
				auto index = m_roots.size();
				m_roots.push_back(e);
				m_entityRootIndexMap.insert({ e, index });
			}

			} else {
				const auto it = m_entityRootIndexMap.find(e);
				if (it != m_entityRootIndexMap.end()) {
					m_roots[it->second] = m_roots.back();
					m_roots.pop_back();
				}
		}
	}

	//INLINE static void _applyTranslationOnly(glm::mat4x4& M, const glm::mat4x4& parentWorld, const glm::vec3& translation) {
	//	M[3] = parentWorld * glm::vec4(translation, 1.0f);
	//}

public:
	~TransformSystem() override {
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

	TransformSystem(ArenaRegistry* const registry, const uint16_t sceneIndex, Arena* const arena)
		: SystemECS_ModelTransformsProvider{ registry, sceneIndex },
		m_entityRootIndexMap{ ArenaAllocator<std::pair<entt::entity, size_t>>{arena} },
		m_roots{ ArenaAllocator<entt::entity>(arena) },
		m_modelTransforms{ ArenaAllocator<ModelTransform>(arena) },
		m_parentOf{ ArenaAllocator<entt::entity>(arena) },
		m_childrenOf{ ArenaAllocator<entt::entity>(arena) },
		m_positions{ArenaAllocator<glm::vec3>{arena}},
		m_rotations{ ArenaAllocator<glm::quat>{arena} },
		m_scales{ ArenaAllocator<glm::vec3>{arena} }
{
		registry->on_destroy<TransformComponent>()
			.connect<&TransformSystem::_onDestroy>(this);

		for (auto& stack : m_threadsStacks)
			stack.reserve(256);

		m_positions.reserve(4096 * 64);
		m_scales.reserve(4096 * 64);
		m_rotations.reserve(4096 * 64);
		m_modelTransforms.reserve(4096 * 64);

		// Start Threads
		for (uint8_t i = 0; i < TRANSFORM_THREADS_MAX; ++i) {
			//m_workers[i] = new std::thread{ TransformSystem::_workerThread, this, i }; 
			//m_workers[i] = new std::thread{ TransformSystem::workerThread_brute_test, this, i };
			//m_workers[i] = new std::thread{ TransformSystem::_workerThread_avx2_test, this, i };

			m_startSemas[i] = new std::binary_semaphore{ 0 };
			m_doneSemas[i] = new std::binary_semaphore{ 0 };
			m_workers[i] = new std::thread{ _workerThread_avx2_test_entities, this, i };
		}
	}

	INLINE ArenaVector<glm::vec3>& positions() { return m_positions; }
	INLINE ArenaVector<glm::quat>& rotations() { return m_rotations; }
	INLINE ArenaVector<glm::vec3>& scales() { return m_scales; }
	
private:
	static constexpr size_t _workerDiv = TRANSFORM_THREADS_MAX * (TRANSFORM_THREADS_MAX / 2);

	INLINE Range<uint32_t> _setupWorkerThreadRange(const entt::entity* const entities, const size_t entCount) {

		const auto startWorkers = static_cast<uint8_t>(
			std::clamp(entCount / _workerDiv,
					   static_cast <size_t>(1),
					   static_cast<size_t>(TRANSFORM_THREADS_MAX)));

		const auto curThreads = m_curNumWorkers.load(std::memory_order_acquire);
		if (curThreads >= TRANSFORM_THREADS_MAX)
			return { 0, 0 };

		const auto availWorkers = std::clamp(static_cast<unsigned int>(startWorkers),
									   static_cast<unsigned int>(0),
									   static_cast<unsigned int>(TRANSFORM_THREADS_MAX) - curThreads);

		uint8_t expected = static_cast<uint8_t>(curThreads + availWorkers);
		m_curNumWorkers.fetch_add(static_cast<uint8_t>(availWorkers), std::memory_order_acq_rel);
		if (!m_curNumWorkers.compare_exchange_strong(
			expected, static_cast<uint8_t>(curThreads + availWorkers))) {
			if (expected >= TRANSFORM_THREADS_MAX)
				return { 0, 0 };
		}

		auto threadRange = Range<uint32_t>{ curThreads, static_cast<uint32_t>(expected) - curThreads };
		const uint32_t offset = static_cast<uint32_t>(
			std::ceil(entCount / static_cast<float>(threadRange.count))
			);


		for (uint32_t i = 0; i < threadRange.count; ++i) {
			const uint32_t start = i * offset;
			const uint32_t count = i == threadRange.count - 1 && entCount % offset != 0
				? entCount % offset
				: offset;

			const auto threadId = threadRange[i];
			m_workerEntities[threadId].reserve(count);
			m_workerEntities[threadId].clear();
			for (size_t e = start; e < start + count; ++e) {
				m_workerEntities[threadRange[i]].push_back(entities[e]);
			}
		}
		
		
		return threadRange;
	}
public:
	INLINE void run(BoundingSystem& boundSys) {

		PROFILE_SCOPE("TransformUpdate");
		auto grp = m_reg->group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
		m_groupPtr = &grp;

		m_boundSysPtr = &boundSys;
		m_curNumWorkers.store(0, std::memory_order_release);
		auto startRange = _setupWorkerThreadRange(m_roots.data(), m_roots.size());

		for (size_t i = startRange.start(); i < startRange.end(); ++i) {
			m_startSemas[i]->release();
		}

		for (size_t i = startRange.start(); i < startRange.end(); ++i) {
			m_doneSemas[i]->acquire();
		}

		////TEST MT
		////static constexpr size_t workerDiv = (TRANSFORM_THREADS_MAX * (TRANSFORM_THREADS_MAX / 2));
		//m_boundSysPtr = &boundSys;
		//auto startWorkers = static_cast<uint8_t>(
		//	std::clamp(m_roots.size() / _workerDiv,
		//			   static_cast <size_t>(1),
		//			   static_cast<size_t>(TRANSFORM_THREADS_MAX)));

		//uint32_t offset = static_cast<uint32_t>(
		//		std::ceil(static_cast<float>(m_roots.size() / static_cast<float>(startWorkers)))
		//	);

		//m_curNumWorkers.store(startWorkers, std::memory_order_acq_rel);
		//for (uint32_t i = 0; i < startWorkers; ++i) {
		//	uint32_t start = i * offset;
		//	uint32_t count = (i == startWorkers - 1 && m_roots.size() % offset != 0)
		//		? m_roots.size() % offset
		//		: offset;
		//	m_workerRanges[i] = { start, count };
		//	m_startSemas[i]->release();
		//}

		//for (size_t i = 0; i < startWorkers; ++i) {
		//	m_doneSemas[i]->acquire();
		//}
	}




	INLINE void registryEmplace(const entt::entity entity, [[maybe_unused]] void* valueInPtr, [[maybe_unused]] void** valueOutPtr) override {
		auto& transComp = m_reg->emplace_or_replace<TransformComponent>(entity, TransformComponent{});
		
		const auto matrixIndex = static_cast<uint32_t>(m_modelTransforms.size());
		transComp.dataIndex = matrixIndex;
		transComp.sceneIndex = m_sceneIndex;
		const auto index = entt::to_integral(entity);
		if (index >= m_parentOf.size())
			m_parentOf.resize(index + 1, entt::null);
		if (index >= m_childrenOf.size())
			m_childrenOf.resize(index + 1);

		//glm::vec3 position{ 0, 0, 0 };
//glm::quat rotation{ 1, 0, 0, 0 };
//glm::vec3 scale{ 1,1,1 };

		m_positions.push_back({ 0, 0, 0 });
		m_rotations.push_back({ 1, 0, 0, 0 });
		m_scales.push_back({ 1,1,1 });
		m_modelTransforms.emplace_back( MMath::composeTRS(m_positions.back(), m_rotations.back(), m_scales.back()) );


		if (valueInPtr) {
			const auto parent = static_cast<entt::entity*>(valueInPtr);
			setParent(entity, parent);
		} else {
			_checkEntityIsRoot(entity);
		}
	}

	INLINE ArenaVector<ModelTransform>& modelTransforms() override {
		return m_modelTransforms;
	}
	INLINE ArenaVector<ModelTransform>& modelTransforms() const override {
		return const_cast<ArenaVector<ModelTransform>&>(m_modelTransforms);
	}

	INLINE void setParent(const entt::entity entity, const entt::entity* parent) {
		const auto id = entt::to_integral(entity);

		if (id >= m_parentOf.size()) {
			m_parentOf.resize(id + 1, entt::null);
			m_childrenOf.resize(id + 1);
		}

		const entt::entity oldParent = m_parentOf[id];
		if (oldParent != entt::null) {
			auto& childrenToOldParent = m_childrenOf[entt::to_integral(oldParent)];
			std::erase(childrenToOldParent, entity);
			_checkEntityIsRoot(oldParent);
		}

		if (parent) {
			const entt::entity newParent = *parent;
			if (newParent == entt::null) {
				m_parentOf[id] = entt::null;
				const auto it = m_entityRootIndexMap.find(entity);
				if (it != m_entityRootIndexMap.end()) {
					m_roots[it->second] = m_roots.back();
					m_roots.pop_back();
				}
				return;
			}

			const auto newParentId = entt::to_integral(newParent);
			if (newParentId >= m_childrenOf.size()) {
				m_childrenOf.resize(newParentId + 1);
			}

			m_parentOf[id] = newParent;
			m_childrenOf[newParentId].push_back(entity);
			_checkEntityIsRoot(newParent);
			_checkEntityIsRoot(entity);

		} else {
			m_parentOf[id] = entt::null;
			const auto it = m_entityRootIndexMap.find(entity);
			if (it != m_entityRootIndexMap.end()) {
				m_roots[it->second] = m_roots.back();
				m_roots.pop_back();
			}
		}
	}

	INLINE std::span<entt::entity> getChildren(const entt::entity ofEntity) {
		const auto id = entt::to_integral(ofEntity);
		if (id >= m_childrenOf.size())
			return std::span<entt::entity>();

		return std::span(m_childrenOf[id].data(), m_childrenOf[id].size());
	}

	INLINE entt::entity getParent(const entt::entity ofEntity) const {
		const auto id = entt::to_integral(ofEntity);
		if (id >= m_parentOf.size())
			return entt::null;
		
		return m_parentOf[id];
	}

	INLINE void clearChildren(const entt::entity parent) {
		const auto parentId = entt::to_integral(parent);
		if (parentId >= m_childrenOf.size()) return;

		for (const auto child : m_childrenOf[parentId]) {
			m_parentOf[entt::to_integral(child)] = entt::null;
		}
		m_childrenOf[parentId].clear();
	}


	INLINE void printHierarchy() {
		Log::logLine(LogType::Info, LogMod::Engine,
					 std::format("\nTransform Hierarchy for Scene index {} - *\n",
					 	static_cast<uint32_t>(m_sceneIndex)) );

		for (const auto root : m_roots) {
			constexpr uint32_t depth = 0;
			logTransformTag(root, depth);
		}
		Log::logLine(LogType::Success, LogMod::Engine, "Done.");
	}

	INLINE void logTransformTag(entt::entity entity, const uint32_t depth) {
		const Transform transform{ m_reg, entity };
		const auto tag = transform.getTag();
		const std::string tagStr = tag.empty() ? "untagged" : std::string{ tag };
		std::string tab = "\t";
		for (size_t i = 0; i < depth; ++i) {
			if (i == depth - 1)
				tab.append("  L ");
			else
				tab.append("\t");
		}
		std::cout << tab << static_cast<uint32_t>(entity) << ", " << tagStr << "\n";
		//LOGLINE_IND(LogType::Info, LogMod::Engine, tagStr, depth);
		for (const auto child : m_childrenOf[entt::to_integral(entity)])
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