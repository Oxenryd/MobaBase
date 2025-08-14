#include "Engine.h"
#include "BoundingVolume.hpp"

AABB BoundingVolume::getCoarseAABB() const {
	BoundingVolumeComponent boundComp = *this;
	auto& transComp = m_reg->get<TransformComponent>(m_entity);

	return Engine::getInstance()->getScene(transComp.sceneIndex)->boundingSystem().aabbs()[boundComp.coarseIndexWorld];

}

AABB BoundingVolume::getCoarseAABB_local() const {
	BoundingVolumeComponent boundComp = *this;
	auto& transComp = m_reg->get<TransformComponent>(m_entity);

	return Engine::getInstance()->getScene(transComp.sceneIndex)->boundingSystem().cachedLocals()[boundComp.coarseIndexLocal];
}