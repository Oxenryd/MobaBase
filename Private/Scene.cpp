#include "Scene.h"
#include "ArenaAllocator.hpp"
#include "TransformComponent.hpp"
#include "Mesh.hpp"
#include "EnabledTag.hpp"
#include "SceneRenderSystem.hpp"
#include "LightSystem.hpp"
#include "TransformSystem.hpp"
#include "GameObjectSystem.hpp"
#include "BVH.hpp"
#include "BoundingSystem.h"
#include <entt/entt.hpp>




SceneBase::SceneBase(const size_t arenaSize, const uint16_t sceneIndex) :
        m_arena{ new Arena{arenaSize} },
        m_sceneIndex{sceneIndex},
        m_reg{ new ArenaRegistry{ArenaAllocator<entt::entity>{m_arena} }},
        m_renderSys{ new SceneRenderSystem{m_reg, m_arena, sceneIndex }},
        m_boundingSys{ new BoundingSystem{m_reg, m_arena, sceneIndex }},
        m_transformSys{ new TransformSystem{m_reg, sceneIndex, m_arena} },
        m_gameObjectSys{ new GameObjectSystem{ m_reg, sceneIndex}},
        m_bvhSys{ new BVHSystem{m_reg}},
        m_lightSys{new LightSystem{m_reg, sceneIndex}}
{
    m_reg->storage<TransformComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
    m_reg->storage<EnabledTag>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
    m_reg->storage<BoundingVolumeComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
    m_reg->storage<MeshFilterComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
    m_reg->group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
    // m_reg.storage<SubMeshComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);

    cullResults.visibleEntities.reserve(1024);
    broadPhaseResults.collisionPairs.reserve(1024);
}
