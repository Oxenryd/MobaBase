#ifndef ALIGNED_HPP
#define ALIGNED_HPP

#include <immintrin.h>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <thread>
#include <algorithm>

constexpr size_t avx_alignment = 32;

template<typename T>
struct AlignedArray
{
    T* data = nullptr;
    size_t size = 0;

    AlignedArray(size_t count)
        : size(count) {
        data = static_cast<T*>(std::aligned_alloc(avx_alignment, sizeof(T) * size));
    }

    ~AlignedArray() {
        std::free(data);
    }

    T& operator[](size_t idx) { return data[idx]; }
    const T& operator[](size_t idx) const { return data[idx]; }
};

#endif