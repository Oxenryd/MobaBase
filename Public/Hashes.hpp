#ifndef HASHES_HPP
#define HASHES_HPP

#include <cstdint>
#include <utility>
#include <functional>
#include <type_traits>

template <typename T>
inline void hash_combine(size_t& seed, const T& val) {
    seed ^= std::hash<T>{}(val)+0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T1, typename T2>
struct PairHash
{
    size_t operator()(const std::pair<T1, T2>& p) const {
        size_t seed = 0;
        hash_combine(seed, p.first);
        hash_combine(seed, p.second);
        return seed;
    }
};


#endif