#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>
#include <type_traits>

#include <glm/glm.hpp>

class SceneBase;

template<typename T>
concept SceneConcept = requires(T t, const size_t size, const uint16_t index, void* arg) {
    { std::is_base_of_v<SceneBase, T> };
    //{ T::create() } -> std::same_as<SceneBase*>;
    { T::create(size, index, arg) } -> std::same_as<SceneBase*>;
};

class GameObject;
template<typename T>
concept GO_Derived = requires
{
    { std::is_base_of_v<GameObject, T> };
};


template<typename T>
concept IsRenderSysCompatible = requires(T t)
{
    {T::afterDraw()} -> std::same_as<void>;
};

template<typename T, size_t N>
concept IsGlmVecCompatible = requires
{
    { std::is_convertible_v<glm::vec<N, float>, T> };
};



namespace ConceptChecks
{
    template<typename T>
    static constexpr bool isValidSceneConcept() {
        if constexpr (
            requires (const size_t s, const uint16_t i, void* a)
        {
            { T::create(s, i, a) } -> std::same_as<SceneBase*>;
        })
            return true;

        return false;
    }
}

#endif