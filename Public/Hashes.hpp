#ifndef HASHES_HPP
#define HASHES_HPP

#include <cstdint>
#include <utility>
#include <functional>
#include <type_traits>

// template <typename T>
// inline void hash_combine(size_t& seed, const T& val) {
//     seed ^= std::hash<T>{}(val)+0x9e3779b9 + (seed << 6) + (seed >> 2);
// }

template <typename T1, typename T2>
struct PairHash
{
    size_t operator()(const std::pair<T1, T2>& p) const {
        size_t seed = 0;
        boost::hash_combine(seed, p.first);
        boost::hash_combine(seed, p.second);
        return seed;
    }
};

template <typename T>
requires std::is_integral_v<T> || std::is_enum_v<T>
struct IntHash {
    size_t operator()(T const& value) const noexcept {
        uint64_t x = static_cast<uint64_t>(value);
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x;
    }
};

template <typename T1, typename T2>
struct FastPairHash
{
    size_t operator()(const std::pair<T1, T2>& p) const {
        size_t seed = 0;
        boost::hash_combine(seed, p.first);
        boost::hash_combine(seed, p.second);
        return seed;
    }
};

struct StringHash {
    using is_transparent = void;

    size_t operator()(std::string_view sv) const noexcept {
        return boost::hash_range(sv.begin(), sv.end());
    }

    size_t operator()(const std::string& s) const noexcept {
        return boost::hash_range(s.begin(), s.end());
    }
    // usage:
    //boost::unordered_flat_map<std::string, Material, StringHash, std::equal_to<>> materialMap;
};


#endif