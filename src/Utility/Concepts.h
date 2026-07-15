#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>
#include <type_traits>

#include "Globals.hpp"


template<typename T>
struct type_tag {
    using type = T;
};
template<typename T>
concept FloatingPointConcept = std::is_floating_point_v<T> && sizeof(T) % 4 == 0;


namespace MMath // FORWARD
{
    template<FloatingPointConcept ValT, size_t N>
        requires (N >= Consts::MIN_VEC_SIZE  && N <= Consts::MAX_VEC_SIZE)       
    struct MVec;
}


class SceneBase; // FORWARD

template<typename T>
concept SceneConcept = requires(T t, const size_t size, const uint16_t index, void* arg) {
    { std::is_base_of_v<SceneBase, T> };
    { T::create(size, index, arg) } -> std::same_as<SceneBase*>;
};

class GameObject; // FORWARD

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
concept IsVecCompatible = 
    std::is_convertible_v<MMath::MVec<float, N>, T> ||
    std::is_convertible_v<MMath::MVec<double, N>, T>;


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
        else
            return false;
    }
}

#endif