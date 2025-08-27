#ifndef MRANDOM_HPP
#define MRANDOM_HPP

#include <cstdint>
#include <random>
#include <type_traits>
#include <chrono>
#include <array>
#include <limits>
#include <glm/glm.hpp>
#include "glm/gtx/quaternion.hpp"

#include "GlobalMacros.h"


//struct MRandom
//{
//private:
//    // SplitMix64 to scramble seed material.
//    static INLINE uint64_t splitmix64_(uint64_t x) {
//        x += 0x9E3779B97F4A7C15ull;
//        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
//        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
//        return x ^ (x >> 31);
//    }
//
//    // Gather some entropy and fold it.
//    static INLINE uint64_t make_seed_() {
//        uint64_t t = static_cast<uint64_t>(
//            std::chrono::steady_clock::now().time_since_epoch().count());
//        uint64_t r1 = 0, r2 = 0;
//        try {
//            std::random_device rd;
//            // std::random_device may be deterministic on some platforms; fold anyway.
//            r1 = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
//            r2 = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
//        } catch (...) {
//            // ignore; fall back to clock only
//        }
//        uint64_t s = t ^ (r1 + 0xA24BAED4963EE407ull) ^ (r2 + 0x9E3779B97F4A7C15ull);
//        return splitmix64_(s);
//    }
//
//    // Per-thread engine; initialized once per thread.
//    static INLINE std::mt19937_64& engine_() {
//        static thread_local std::mt19937_64 eng{ make_seed_() };
//        return eng;
//    }
//
//    // Helpers for clamping/validating ranges.
//    template <class T>
//    static INLINE void ensure_order_(T& a, T& b) {
//        if (a > b) std::swap(a, b);
//    }
//
//public:
//    // --- Seeding / state control ---
//    static INLINE void reseed(uint64_t seed) {
//        engine_().seed(seed);
//    }
//    static INLINE uint64_t reseed_random() {
//        auto s = make_seed_();
//        engine_().seed(s);
//        return s;
//    }
//
//    // --- Unsigned integers (full-width 0..max) ---
//    static INLINE uint8_t  nextU8() { return static_cast<uint8_t>(std::uniform_int_distribution<uint16_t>(0, std::numeric_limits<uint8_t >::max())(engine_())); }
//    static INLINE uint16_t nextU16() { return static_cast<uint16_t>(std::uniform_int_distribution<uint32_t>(0, std::numeric_limits<uint16_t>::max())(engine_())); }
//    static INLINE uint32_t nextU32() { return std::uniform_int_distribution<uint32_t>(0u, std::numeric_limits<uint32_t>::max())(engine_()); }
//    static INLINE uint64_t nextU64() { return std::uniform_int_distribution<uint64_t>(0ull, std::numeric_limits<uint64_t>::max())(engine_()); }
//
//    // --- Reals in [0,1) ---
//    static INLINE float  nextFloat01() {
//        static thread_local std::uniform_real_distribution<float>  d(0.0f, 1.0f);
//        return d(engine_());
//    }
//    static INLINE double nextDouble01() {
//        static thread_local std::uniform_real_distribution<double> d(0.0, 1.0);
//        return d(engine_());
//    }
//
//    // --- Reals in [min,max] (order-agnostic) ---
//    static INLINE float nextFloat(float min, float max) {
//        ensure_order_(min, max);
//        std::uniform_real_distribution<float> d(min, std::nextafter(max, std::numeric_limits<float>::infinity()));
//        return d(engine_());
//    }
//    static INLINE double nextDouble(double min, double max) {
//        ensure_order_(min, max);
//        std::uniform_real_distribution<double> d(min, std::nextafter(max, std::numeric_limits<double>::infinity()));
//        return d(engine_());
//    }
//
//    // --- Signed integers in [min,max] (order-agnostic) ---
//    //template <class Int,
//    //    std::enable_if_t<std::is_integral_v<Int>&& std::is_signed_v<Int>, int> = 0>
//    //static INLINE Int nextInt(Int min, Int max) {
//    //    ensure_order_(min, max);
//    //    std::uniform_int_distribution<Int> d(min, max);
//    //    return d(engine_());
//    //}
//    template <class Int,
//        std::enable_if_t<std::is_integral_v<Int>&& std::is_signed_v<Int>, int> = 0>
//    static inline Int nextInt(Int min, Int max) {
//        ensure_order_(min, max);
//        using B = DistBaseT<Int>;
//        std::uniform_int_distribution<B> d(static_cast<B>(min), static_cast<B>(max));
//        return static_cast<Int>(d(engine_()));
//    }
//
//    // Convenience overloads for common sizes
//    static INLINE int8_t  nextI8(int8_t  min, int8_t  max) { return nextInt<int8_t >(min, max); }
//    static INLINE int16_t nextI16(int16_t min, int16_t max) { return nextInt<int16_t>(min, max); }
//    static INLINE int32_t nextI32(int32_t min, int32_t max) { return nextInt<int32_t>(min, max); }
//    static INLINE int64_t nextI64(int64_t min, int64_t max) { return nextInt<int64_t>(min, max); }
//
//    // --- Gaussian / Normal ---
//    // float version
//    static INLINE float nextGaussianF(const float& mu = 0.0f, const float& sigma = 1.0f) {
//        // Separate per-thread distribution instance to cache params.
//        static thread_local std::normal_distribution<float> dist;
//        // std::normal_distribution doesn't have set_params for float directly before C++20 on some libs;
//        // construct on the fly with desired params:
//        std::normal_distribution<float> d(mu, sigma);
//        return d(engine_());
//    }
//    // double version
//    static INLINE double nextGaussian(const double& mu = 0.0, const double& sigma = 1.0) {
//        std::normal_distribution<double> d(mu, sigma);
//        return d(engine_());
//    }
//
//    static INLINE glm::vec3 nextDir() {
//        return glm::normalize(glm::vec3{nextFloat(-1.0f, 1.0f), nextFloat(-1.0f, 1.0f),nextFloat(-1.0f, 1.0f) });
//    }
//
//    static INLINE glm::vec3 nextVector(float minLength, float maxLength) {
//        const auto len = nextFloat(minLength, maxLength);
//        return nextDir() * len;
//    }
//
//    static INLINE glm::quat nextRotation() {
//        auto anglesDeg = glm::vec3{nextFloat(0.0f, 360.0f), nextFloat(0.0f, 360.0f) , nextFloat(0.0f, 360.0f) };
//        glm::quat{ glm::radians(anglesDeg) };
//    }
//};

struct MRandom
{
private:
    static inline uint64_t splitmix64_(uint64_t x) {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }
    static inline uint64_t make_seed_() {
        uint64_t t = static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        std::random_device rd;
        uint64_t r1 = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
        uint64_t r2 = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
        return splitmix64_(t ^ (r1 + 0xA24BAED4963EE407ull) ^ (r2 + 0x9E3779B97F4A7C15ull));
    }
    static inline std::mt19937_64& engine_() {
        static thread_local std::mt19937_64 eng{ make_seed_() };
        return eng;
    }
    template <class T>
    static inline void ensure_order_(T& a, T& b) { if (a > b) std::swap(a, b); }

    // Map Int -> a standard-compliant distribution base type.
    template <class Int, bool Signed = std::is_signed_v<Int>>
    struct DistBase_;
    // Signed mapping
    template <class Int>
    struct DistBase_<Int, true>
    {
        using type = std::conditional_t<
            (sizeof(Int) <= sizeof(int)), int,
            std::conditional_t<(sizeof(Int) <= sizeof(long)), long, long long>>;
    };
    // Unsigned mapping
    template <class Int>
    struct DistBase_<Int, false>
    {
        using type = std::conditional_t<
            (sizeof(Int) <= sizeof(unsigned int)), unsigned int,
            std::conditional_t<(sizeof(Int) <= sizeof(unsigned long)), unsigned long, unsigned long long>>;
    };
    template <class Int>
    using DistBaseT = typename DistBase_<Int>::type;

public:
    // Seeding
    static inline void reseed(uint64_t seed) { engine_().seed(seed); }
    static inline uint64_t reseed_random() { auto s = make_seed_(); engine_().seed(s); return s; }

    // Unsigned full-width
    static inline uint8_t  nextU8() {
        using B = DistBaseT<uint8_t>;
        std::uniform_int_distribution<B> d(0, static_cast<B>(std::numeric_limits<uint8_t>::max()));
        return static_cast<uint8_t>(d(engine_()));
    }
    static inline uint16_t nextU16() {
        using B = DistBaseT<uint16_t>;
        std::uniform_int_distribution<B> d(0, static_cast<B>(std::numeric_limits<uint16_t>::max()));
        return static_cast<uint16_t>(d(engine_()));
    }
    static inline uint32_t nextU32() {
        using B = DistBaseT<uint32_t>;
        std::uniform_int_distribution<B> d(0, static_cast<B>(std::numeric_limits<uint32_t>::max()));
        return static_cast<uint32_t>(d(engine_()));
    }
    static inline uint64_t nextU64() {
        using B = DistBaseT<uint64_t>;
        std::uniform_int_distribution<B> d(0, static_cast<B>(std::numeric_limits<uint64_t>::max()));
        return static_cast<uint64_t>(d(engine_()));
    }

    // Reals in [0,1)
    static inline float  nextFloat01() {
        static thread_local std::uniform_real_distribution<float>  d(0.0f, 1.0f);
        return d(engine_());
    }
    static inline double nextDouble01() {
        static thread_local std::uniform_real_distribution<double> d(0.0, 1.0);
        return d(engine_());
    }

    // Reals in [min,max] (closed interval)
    static inline float nextFloat() { return nextFloat01(); }
    static inline float nextFloat(float min, float max) {
        ensure_order_(min, max);
        std::uniform_real_distribution<float> d(min, std::nextafter(max, std::numeric_limits<float>::infinity()));
        return d(engine_());
    }
    static inline float nextDouble() { return nextDouble01(); }
    static inline double nextDouble(double min, double max) {
        ensure_order_(min, max);
        std::uniform_real_distribution<double> d(min, std::nextafter(max, std::numeric_limits<double>::infinity()));
        return d(engine_());
    }

    // Signed ints in [min,max] — works for int8_t/int16_t/etc. on MSVC
    template <class Int,
        std::enable_if_t<std::is_integral_v<Int>&& std::is_signed_v<Int>, int> = 0>
    static inline Int nextInt(Int min, Int max) {
        ensure_order_(min, max);
        using B = DistBaseT<Int>;
        std::uniform_int_distribution<B> d(static_cast<B>(min), static_cast<B>(max));
        return static_cast<Int>(d(engine_()));
    }

    // Convenience overloads
    static inline int8_t  nextI8(int8_t  min, int8_t  max) { return nextInt<int8_t >(min, max); }
    static inline int16_t nextI16(int16_t min, int16_t max) { return nextInt<int16_t>(min, max); }
    static inline int32_t nextI32(int32_t min, int32_t max) { return nextInt<int32_t>(min, max); }
    static inline int64_t nextI64(int64_t min, int64_t max) { return nextInt<int64_t>(min, max); }

    // Gaussian / Normal
    static inline float  nextGaussianF(const float& mu = 0.0f, const float& sigma = 1.0f) {
        std::normal_distribution<float>  d(mu, sigma);
        return d(engine_());
    }
    static inline double nextGaussian(const double& mu = 0.0, const double& sigma = 1.0) {
        std::normal_distribution<double> d(mu, sigma);
        return d(engine_());
    }

    static INLINE glm::vec3 nextDir() {
        return glm::normalize(glm::vec3{ nextFloat(-1.0f, 1.0f), nextFloat(-1.0f, 1.0f),nextFloat(-1.0f, 1.0f) });
    }

    static INLINE glm::vec3 nextVector(float minLength, float maxLength) {
        const auto len = nextFloat(minLength, maxLength);
        return nextDir() * len;
    }

    static INLINE glm::quat nextRotation() {
        auto anglesDeg = glm::vec3{ nextFloat(-360.0f, 360.0f), nextFloat(-360.0f, 360.0f) , nextFloat(-360.0f, 360.0f) };
        return glm::quat{ glm::radians(anglesDeg) };

    }

};

#endif