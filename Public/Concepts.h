#ifndef CONCEPTS_H
#define CONCEPTS_H

class SceneBase;

template<typename T>
concept SceneConcept = requires(T t, void* arg, double dt, uint16_t index, size_t size) {
    { std::is_base_of_v<SceneBase, T> };
    { T::createDefault(size, index, arg) } -> std::convertible_to<SceneBase*>;
};


#endif