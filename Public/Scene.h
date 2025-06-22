#ifndef SCENE_H
#define SCENE_H

#include <cstdint>
#include <entt/entt.hpp>
#include <concepts>
#include <type_traits>

#include "Arena.hpp"
#include "ArenaAllocator.hpp"

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

using HeapArenaEnttRegistry = entt::basic_registry<
	entt::entity,
	HeapArenaAllocator<entt::entity>>;

class SceneBase
{
private:
    static constexpr size_t DEFAULT_HEAP_SIZE = 64 * 1024 * 1024;

protected:
    HeapArena m_heap;
    HeapArenaEnttRegistry m_reg;

public:
    virtual ~SceneBase() = default;
    SceneBase(size_t heapSize) :
        m_heap(heapSize),
        m_reg{ HeapArenaAllocator<entt::entity>(&m_heap) } {}
    SceneBase() : SceneBase(DEFAULT_HEAP_SIZE) {}

    HeapArena& heapArena() { return m_heap; }

    virtual void updateDispatch(double deltaTime) {}
    virtual void lateUpdateDispatch(double deltaTime) {}
    virtual void fixedUpdateDispatch() {}
};

template<typename Derived>
class Scene : public SceneBase
{
public:
    virtual ~Scene() {}
    Scene() : SceneBase() {}
    Scene(size_t heapSize) : SceneBase(heapSize) {}
    static SceneBase* defaultCreation(void* arg) {
        return new Derived{};
    }

    void load() {
        if constexpr (requires (Derived & d) { d.load(); }) {
            static_cast<Derived&>(*this).load();
        }
    }

    void start() {
        if constexpr (requires (Derived & d) { d.start(); }) {
            static_cast<Derived&>(*this).start();
        }
    }

    void updateDispatch(double dt) override {
        if constexpr (requires (Derived & d) { d.update(dt); }) {
            static_cast<Derived&>(*this).update(dt);
        }
    }

    void lateUpdateDispatch(double dt) override {
        if constexpr (requires (Derived & d) { d.lateUpdate(dt); }) {
            static_cast<Derived&>(*this).lateUpdate(dt);
        }
    }

    void fixedUpdateDispatch() override {
        if constexpr (requires (Derived & d) { d.fixedUpdate(); }) {
            static_cast<Derived&>(*this).fixedUpdate();
        }
    }

    void transitionEnter() {
        if constexpr (requires (Derived & d) { d.transitionEnter(); }) {
            static_cast<Derived&>(*this).transitionEnter();
        }
    }

    SceneTransitionStatus transitioning() {
        if constexpr (requires (Derived & d) {
            { d.transitioning() } -> std::convertible_to<SceneTransitionStatus>;
        }) {
            return static_cast<Derived&>(*this).transitioning();
        } else {
            return SceneTransitionStatus::Done;
        }
    }

    void transitionExit() {
        if constexpr (requires (Derived & d) { d.transitionExit(); }) {
            static_cast<Derived&>(*this).transitionExit();
        }
    }

    SceneBase* createDefault() {
        if constexpr (requires (Derived & d) { { d.createDefault() } -> std::convertible_to<SceneBase*>;
        }) {
            return static_cast<Derived&>(*this).createDefault();
        } else
            return nullptr;
    }
};

class DefaultScene : public Scene<DefaultScene>
{
public:
    static SceneBase* createDefault(void* arg) {
        return new DefaultScene{};
    }
};

template<typename T>
concept SceneConcept = requires(T t, void* arg, double dt) {
    { std::is_base_of_v<SceneBase, T> };
    { T::createDefault(arg) } -> std::convertible_to<SceneBase*>;
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