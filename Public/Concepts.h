#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>
#include <type_traits>

#include <glm/glm.hpp>

class SceneBase;

template<typename T>
concept SceneConcept = requires(T t, void* arg, double dt, uint16_t index, size_t size) {
    { std::is_base_of_v<SceneBase, T> };
    { T::createDefault(size, index, arg) } -> std::convertible_to<SceneBase*>;
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
#endif