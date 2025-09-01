#include "BoundingSystem.h"
#include "Profiler.hpp"
#include "MWork.hpp"
#include "TransformComponent.hpp"
#include "BoundingVolume.hpp"
#include "Engine.h"

void BoundingSystem::run() {
	auto grp = m_reg->group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
	
	MWork::for_loop(0, grp.size(), 16,
					[&](size_t i)
					{
						entt::entity e = grp[i];
						auto& tr = grp.get<TransformComponent>(e);
						if (tr.state.hasFlag(ObjectState::MovedThisFrame) ||
							tr.state.hasFlag(ObjectState::ParentMovedThisFrame)) {

							
							// Has moved. update aabb
							auto& bv = grp.get<BoundingVolumeComponent>(e);
							const auto& localAABB = cachedLocals()[bv.coarseIndexLocal];
							aabbs()[bv.coarseIndexWorld] =
								localAABB.transformed_noPerspective(
									Engine::getInstance()->getScene(tr.sceneIndex)->transformSystem().modelTransforms()[tr.dataIndex]);

						}
					});

	}