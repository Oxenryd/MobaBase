#ifndef SCENE_H
#define SCENE_H

//#include <entt/entt.hpp>

//#include "ArenaAllocator.hpp"
//#include "TransformSystem.hpp"
//#include "GameObjectSystem.hpp"
#include "SceneRenderSystem.hpp"
//#include "BVH.hpp"

#include "BasicTypes.hpp"
#include "Concepts.h"

class Engine;
class SceneRenderSystem;
class TransformSystem;
class BoundingSystem;
class BVHSystem;
class LightSystem;
class GameObjectSystem;

enum class SceneTransitionStatus
{
	None = 0,
	Pending,
	Transitioning,
	Done
};

enum class SceneTransitionMode : uint8_t
{
	DontRunTransitioning = 0,
	RunOnce = 1,
	WaitForDone = 2
	
};


class SceneBase
{
    friend Engine;
    static constexpr size_t DEFAULT_HEAP_SIZE = 64 * 1024 * 1024;

protected:
    //HeapArena m_heap;
    Arena* m_arena;
    uint16_t m_sceneIndex;
    bool m_pendingUnload = false;
    bool m_firstFrame = true;
    ArenaRegistry* m_reg;
    SceneRenderSystem* m_renderSys;
    BoundingSystem* m_boundingSys;
    TransformSystem* m_transformSys;
    GameObjectSystem* m_gameObjectSys;
    BVHSystem* m_bvhSys;
    LightSystem* m_lightSys;

    //template<typename T>
    //void _registerComponent() {
    //    m_reg.storage<T>(ArenaAllocator<T>{&m_arena}).reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
    //}

public:
    ~SceneBase() {};
    SceneBase(const SceneBase&) = delete;
    SceneBase& operator=(const SceneBase&) = delete;
    SceneBase(SceneBase&&) = delete;
    SceneBase& operator=(SceneBase&&) = delete;
    SceneBase(size_t arenaSize, uint16_t sceneIndex);
    //SceneBase() : SceneBase(DEFAULT_HEAP_SIZE) {}

    //HeapArena& heapArena() { return m_heap; }
    const bool& isFirstFrame() const { return m_firstFrame; }


    virtual void updateDispatch(const double) {}
    virtual void lateUpdateDispatch(const double) {}
    virtual void fixedUpdateDispatch() {}
    virtual void loadDispatch() {}
    virtual void startDispatch() {}
    virtual void unloadDispatch() {}
    virtual SceneTransitionStatus transitioningDispatch() { return SceneTransitionStatus::Done; }

    // Base Systems accessors
    ArenaRegistry& registry() { return *m_reg; }
    TransformSystem& transformSystem() { return *m_transformSys; }
    GameObjectSystem& gameObjectSystem() { return *m_gameObjectSys; }
    BoundingSystem& boundingSystem() { return *m_boundingSys; }
    SceneRenderSystem& sceneRender() { return *m_renderSys; }
    LightSystem& lightSystem() { return *m_lightSys; }
    BVHSystem& bvhSystem() { return *m_bvhSys; }
    void setUnload() { m_pendingUnload = true; }
    uint16_t sceneIndex() const { return m_sceneIndex; }
    bool pendingUnload() const { return m_pendingUnload; }

    TraversalResult cullResults;
    TraversalResult broadPhaseResults;
};


template<typename Derived>
class Scene : public SceneBase
{
public:
    ~Scene() {
        static_assert(ConceptChecks::isValidSceneConcept<Derived>(), "Invalid SceneBase");
    }
    Scene() = delete;
    Scene(const size_t arenaSize, const uint16_t index) : SceneBase(arenaSize, index) {}

    void loadDispatch() override {
        if constexpr (requires (Derived& d) { d.load(); }) {
            static_cast<Derived&>(*this).m_firstFrame = true;
            static_cast<Derived&>(*this).load();
        }
    }

    void unloadDispatch() override {
        if constexpr (requires (Derived& d) { d.unload(); }) {
            static_cast<Derived&>(*this).unload();
        }
    }

    void startDispatch() override {
        m_firstFrame = false;
        if constexpr (requires (Derived& d) { d.start(); }) {
            static_cast<Derived&>(*this).start();
        }
    }

    void updateDispatch(double dt) override {          
        if constexpr (requires (Derived& d) { d.update(dt); }) {
            static_cast<Derived&>(*this).update(dt);
            static_cast<Derived&>(*this).m_firstFrame = false;
        }
        //m_transformSys.run(m_boundingSys);
        //m_gameObjectSys.run();
    }

    void lateUpdateDispatch(double dt) override {
        if constexpr (requires (Derived & d) { d.lateUpdate(dt); }) {
            static_cast<Derived&>(*this).lateUpdate(dt);
        }

        m_renderSys->afterDraw();
    }

    void fixedUpdateDispatch() override {
        if constexpr (requires (Derived& d) { d.fixedUpdate(); }) {
            static_cast<Derived&>(*this).fixedUpdate();
        }
    }

    SceneTransitionStatus transitioningDispatch() override {
        if constexpr (requires (Derived& d) {
            { d.transitioning() } -> std::convertible_to<SceneTransitionStatus>;
        }) {
            return static_cast<Derived&>(*this).transitioning();
        } else {
            return SceneTransitionStatus::Done;
        }
    }
};


class DefaultScene final : public Scene<DefaultScene>
{
public:
    DefaultScene(const size_t arenaSize, const uint16_t index)
        : Scene{ arenaSize, index } {}

    static SceneBase* create(const size_t arenaSize, const uint16_t index, void*) {
        return new DefaultScene{ arenaSize, index};
    }
};


#endif