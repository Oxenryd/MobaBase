#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>
#include <type_traits>

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



#endif