#ifndef ARENA_HPP
#define ARENA_HPP

#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>
#include <algorithm>

// Simple Arena Allocator
class Arena
{
public:
    Arena(size_t size)
        : m_size(size), m_offset(0) {
        m_memory = new uint8_t[size];
    }

    ~Arena() {
        delete[] m_memory;
    }

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        size_t current = reinterpret_cast<size_t>(m_memory) + m_offset;
        size_t aligned = alignUp(current, alignment);
        size_t offset = aligned - reinterpret_cast<size_t>(m_memory);

        if (offset + size > m_size) {
            // Out of memory
            return nullptr;
        }

        void* ptr = m_memory + offset;
        m_offset = offset + size;
        return ptr;
    }

    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    void reset() {
        m_offset = 0;
    }

    size_t used() const { return m_offset; }
    size_t capacity() const { return m_size; }

private:
    uint8_t* m_memory;
    size_t m_size;
    size_t m_offset;

    static size_t alignUp(size_t addr, size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
};

#endif