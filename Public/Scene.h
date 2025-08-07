#ifndef SCENE_H
#define SCENE_H

#include <cstdint>
#include <entt/entt.hpp>
#include <concepts>
#include <type_traits>

#include "ArenaAllocator.hpp"
#include "TransformSystem.hpp"
#include "GameObjectSystem.hpp"
#include "TagSystem.hpp"
#include "SceneRenderSystem.hpp"
#include "BoundingSystem.h"

enum class SceneTransitionStatus
{
	None = 0,
	Pending,
	Transitioning,
	Done
};

enum class SceneTransitionMode
{
	DontRunTransitioning = 0,
	RunOnce = 1,
	WaitForDone = 2
	
};

class Engine;
class SceneBase
{
private:
    friend Engine;
    static constexpr size_t DEFAULT_HEAP_SIZE = 64 * 1024 * 1024;

protected:
    //HeapArena m_heap;
    Arena m_arena;
    uint16_t m_sceneIndex;
    bool m_pendingUnload = false;
    bool m_firstFrame = true;
    ArenaRegistry m_reg;
    SceneRenderSystem m_renderSys;
    //TagSystem m_nameTagSys;
    TransformSystem m_transformSys;
    GameObjectSystem m_gameObjectSys;
    BoundingSystem m_boundingSys;
    
    
    
    
    //template<typename T>
    //void _registerComponent() {
    //    m_reg.storage<T>(ArenaAllocator<T>{&m_arena}).reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
    //}

public:
    virtual ~SceneBase() = default;
    SceneBase(const SceneBase&) = delete;
    SceneBase& operator=(const SceneBase&) = delete;
    SceneBase(SceneBase&&) = delete;
    SceneBase& operator=(SceneBase&&) = delete;
    SceneBase(size_t arenaSize, uint16_t sceneIndex) :
        
        m_arena{ arenaSize },
        m_reg{ ArenaAllocator<entt::entity>{&m_arena} },
        m_transformSys{&m_reg, sceneIndex, &m_arena},
        m_gameObjectSys{ sceneIndex, &m_reg},
        m_renderSys{ &m_reg, &m_arena, sceneIndex },
        m_boundingSys{&m_reg, &m_arena, sceneIndex },
        m_sceneIndex{sceneIndex}
    {
        m_reg.storage<TransformComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
        m_reg.storage<EnabledTag>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
        m_reg.storage<BoundingVolumeComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
        m_reg.storage<MeshComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
        m_reg.storage<SubMeshComponent>().reserve(ECS_BASE_COMPONENTS_RESERVATION_COUNT);
    }
    //SceneBase() : SceneBase(DEFAULT_HEAP_SIZE) {}

    //HeapArena& heapArena() { return m_heap; }
    const bool& isFirstFrame() const { return m_firstFrame; }


    virtual void updateDispatch(double deltaTime) {}
    virtual void lateUpdateDispatch(double deltaTime) {}
    virtual void fixedUpdateDispatch() {}
    virtual void loadDispatch() {}
    virtual void startDispatch() {}
    virtual void unloadDispatch() {}
    virtual SceneTransitionStatus transitioningDispatch() { return SceneTransitionStatus::Done; }

    // Base Systems accessors
    ArenaRegistry& registry() { return m_reg; }
    TransformSystem& transformSystem() { return m_transformSys; }
    GameObjectSystem& gameObjectSystem() { return m_gameObjectSys; }  
    BoundingSystem& boundingSystem() { return m_boundingSys; }
    SceneRenderSystem& sceneRender() { return m_renderSys; }
    void setUnload() { m_pendingUnload = true; }
    uint32_t sceneIndex() const { return m_sceneIndex; }
    bool pendingUnload() const { return m_pendingUnload; }
};

template<typename Derived>
class Scene : public SceneBase
{
public:
    virtual ~Scene() {}
    Scene() = delete;
    Scene(size_t arenaSize, uint32_t index) : SceneBase(arenaSize, index) {}
    //static SceneBase* defaultCreation(void* arg) {
    //    return new Derived{};
    //}

    void loadDispatch() override {
        if constexpr (requires (Derived & d) { d.load(); }) {
            static_cast<Derived&>(*this).m_firstFrame = true;
            static_cast<Derived&>(*this).load();
        }
    }

    void unloadDispatch() override {
        if constexpr (requires (Derived & d) { d.unload(); }) {
            static_cast<Derived&>(*this).unload();
        }
    }

    void startDispatch() override {
        m_firstFrame = false;
        if constexpr (requires (Derived & d) { d.start(); }) {
            static_cast<Derived&>(*this).start();
        }
    }

    void updateDispatch(double dt) override {          
        if constexpr (requires (Derived & d) { d.update(dt); }) {
            static_cast<Derived&>(*this).update(dt);
            static_cast<Derived&>(*this).m_firstFrame = false;
        }
        m_transformSys.run();
        m_gameObjectSys.run();
    }

    void lateUpdateDispatch(double dt) override {
        if constexpr (requires (Derived & d) { d.lateUpdate(dt); }) {
            static_cast<Derived&>(*this).lateUpdate(dt);
        }
        m_renderSys.afterDraw();
    }

    void fixedUpdateDispatch() override {
        if constexpr (requires (Derived & d) { d.fixedUpdate(); }) {
            static_cast<Derived&>(*this).fixedUpdate();
        }
    }

    SceneTransitionStatus transitioningDispatch() override {
        if constexpr (requires (Derived & d) {
            { d.transitioning() } -> std::convertible_to<SceneTransitionStatus>;
        }) {
            return static_cast<Derived&>(*this).transitioning();
        } else {
            return SceneTransitionStatus::Done;
        }
    }
};

class DefaultScene : public Scene<DefaultScene>
{
public:
    DefaultScene(size_t arenaSize, uint32_t index)
        : Scene<DefaultScene>{ arenaSize, index } {}
    static SceneBase* createDefault(size_t arenaSize, uint32_t index, void* arg) {
        return new DefaultScene{ arenaSize, index};
    }
};

template<typename T>
concept SceneConcept = requires(T t, void* arg, double dt, uint32_t index, size_t size) {
    { std::is_base_of_v<SceneBase, T> };
    { T::createDefault(size, index, arg) } -> std::convertible_to<SceneBase*>;
};


//class SceneHandle
//{
//public:
//    using UpdateFn = void(*)(void*, double);
//    using LateUpdateFn = void(*)(void*, double);
//    using TransitioningFn = SceneTransitionStatus(*)(void*);
//    using DestroyFn = void(*)(void*);
//
//    SceneHandle() = default;
//
//    SceneHandle(
//        void* instance,
//        UpdateFn updateFn,
//        LateUpdateFn lateUpdateFn,
//        TransitioningFn transitioningFn,
//        DestroyFn destroyFn
//    ) :
//        m_instance(instance),
//        m_update(updateFn),
//        m_lateUpdate(lateUpdateFn),
//        m_transitioning(transitioningFn),
//        m_destroy(destroyFn) {}
//
//    // Dispatcher methods
//    void update(double dt) const {
//        if (m_update) m_update(m_instance, dt);
//    }
//
//    void lateUpdate(double dt) const {
//        if (m_lateUpdate) m_lateUpdate(m_instance, dt);
//    }
//
//    SceneTransitionStatus transitioning() const {
//        return m_transitioning ? m_transitioning(m_instance)
//            : SceneTransitionStatus::Done;
//    }
//
//    void destroy() {
//        if (m_destroy) {
//            m_destroy(m_instance);
//            m_instance = nullptr;
//        }
//    }
//
//    void* raw() const { return m_instance; }
//
//private:
//    void* m_instance = nullptr;
//    UpdateFn m_update = nullptr;
//    LateUpdateFn m_lateUpdate = nullptr;
//    TransitioningFn m_transitioning = nullptr;
//    DestroyFn m_destroy = nullptr;
//};


#endif