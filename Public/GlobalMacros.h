#ifndef GLOBALMACROS_H
#define GLOBALMACROS_H

#ifndef INLINE
    #if defined(__clang__)
        #define INLINE inline __attribute__((always_inline))
    #elif defined(__GNUC__) || defined(__GNUG__)
        #define INLINE [[gnu::always_inline]] inline
    #elif defined(_MSC_VER)
        #define INLINE __forceinline
    #endif
#endif

#include "Log.hpp"

#define UINT8_INVALID static_cast<uint8_t>(0xff)
#define UINT8_SAFEMAX static_cast<uint8_t>(0xfe)
#define UINT16_INVALID static_cast<uint16_t>(0xffff)
#define UINT16_SAFEMAX static_cast<uint16_t>(0xfffe)
#define UINT32_INVALID static_cast<uint32_t>(0xffffffff)
#define UINT32_SAFEMAX static_cast<uint32_t>(0xfffffffe)
#define UINT64_INVALID static_cast<uint64_t>(0xffffffffffffffff)
#define UINT64_SAFEMAX static_cast<uint64_t>(0xfffffffffffffffe)
#define SIZE_INVALID static_cast<size_t>(0xffffffffffffffff)
#define SIZE_SAFEMAX static_cast<size_t>(0xfffffffffffffffe)

constexpr std::size_t operator"" _KB(unsigned long long val) {
    return val * 1024ULL;
}

constexpr std::size_t operator"" _MB(unsigned long long val) {
    return val * 1024ULL * 1024ULL;
}

constexpr std::size_t operator"" _GB(unsigned long long val) {
    return val * 1024ULL * 1024ULL * 1024ULL;
}
constexpr std::size_t operator"" _TB(unsigned long long val) {
    return val * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
}

//#define TB(x) static_cast<size_t>(1024 * 1024 * 1024 * 1024 * (x))
//#define GB(x) static_cast<size_t>(1024 * 1024 * 1024 * (x))
//#define MB(x) static_cast<size_t>(1024 * 1024 * (x))
//#define KB(x) static_cast<size_t>(1024 * (x))

template<typename T>
inline constexpr T& deref(void* ptr, const char* errorMsg = "null pointer in deref") {
    if (!ptr) {
        LOGLINE(LogType::Error, LogMod::Memory, errorMsg);
        assert(false && errorMsg);
    }
    return *static_cast<T*>(ptr);
}
template<typename T>
inline constexpr const T& deref(const void* ptr, const char* errorMsg = "null pointer in deref") {
    if (!ptr) {
        LOGLINE(LogType::Error, LogMod::Memory, errorMsg);
        assert(false && errorMsg);
    }
    return *static_cast<const T*>(ptr);
}
#endif