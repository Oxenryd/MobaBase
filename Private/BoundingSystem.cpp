#include "BoundingSystem.h"
#include "Profiler.hpp"
#include "MJob.hpp"
#include "TransformComponent.hpp"
#include "BoundingVolume.hpp"
#include "Engine.h"
#include "Scene.h"
#include "TransformSystem.hpp"

void BoundingSystem::run() {
	const auto grp = m_reg->group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
	
	MJob::for_loop(0, grp.size(), 16,
					[&](const size_t i)
					{
						const entt::entity e = grp[i];
						const auto& tr = grp.get<TransformComponent>(e);
						if (tr.state.hasFlag(ObjectState::MovedThisFrame) ||
							tr.state.hasFlag(ObjectState::ParentMovedThisFrame)) {

							
							// Has moved. update aabb
							const auto& bv = grp.get<BoundingVolumeComponent>(e);
							const auto& localAABB = cachedLocals()[bv.coarseIndexLocal];
							aabbs()[bv.coarseIndexWorld] =
								localAABB.transformed_noPerspective(
									Engine::getInstance()->getScene(tr.sceneIndex)->
									transformSystem().modelTransforms()[tr.dataIndex]
								);

						}
					});

	}