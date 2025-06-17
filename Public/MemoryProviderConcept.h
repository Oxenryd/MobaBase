
#ifndef MEMORYPROVIDER_CONCEPT_H
#define MEMORYPROVIDER_CONCEPT_H

#include <concepts>

template <typename T>
concept IsMemoryProvider = requires  {
    { std::declval<T>().registerArena() } -> std::same_as<uint32_t>;
};

#endif // !MEMORYPROVIDER_CONCEPT_H
