#include "Engine.h"
#include "BoundingVolume.hpp"
#include "TransformComponent.hpp"
#include "Scene.h"

void BoundingVolume::setFlags(BoundingVolumeFlags flags) {
	BoundingVolumeComponent& boundComp = *this;
	boundComp.flags = static_cast<uint32_t>(flags);
}

void BoundingVolume::setCoarseAABB(const AABB& aabb) {
	BoundingVolumeComponent& boundComp = *this;
	const auto& transComp = m_reg->get<TransformComponent>(m_entity);

	Engine::getInstance()->getScene(transComp.sceneIndex)->boundingSystem().aabbs()[boundComp.coarseIndexWorld] = aabb;
}

void BoundingVolume::setCoarseAABB_local(const AABB& aabb) {
	BoundingVolumeComponent& boundComp = *this;
	const auto& transComp = m_reg->get<TransformComponent>(m_entity);

	Engine::getInstance()->getScene(transComp.sceneIndex)->boundingSystem().aabbs()[boundComp.coarseIndexLocal] = aabb;
}

AABB BoundingVolume::getCoarseAABB() const {
	const BoundingVolumeComponent& boundComp = m_reg->get<BoundingVolumeComponent>(m_entity);
	const auto& transComp = m_reg->get<TransformComponent>(m_entity);

	return Engine::getInstance()->getScene(transComp.sceneIndex)->boundingSystem().aabbs()[boundComp.coarseIndexWorld];

}

AABB BoundingVolume::getCoarseAABB_local() const {
	BoundingVolumeComponent& boundComp = *this;
	const auto& transComp = m_reg->get<TransformComponent>(m_entity);

	return Engine::getInstance()->getScene(transComp.sceneIndex)->boundingSystem().cachedLocals()[boundComp.coarseIndexLocal];
}