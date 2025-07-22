#ifndef ARENAALLOCATOR_HPP
#define ARENAALLOCATOR_HPP

#include <cstdint>

class Arena;
class HeapArena;

#ifndef ARENA_HPP
    #include "Arena.hpp"
#endif

template<typename T>
class HeapArenaAllocator
{
public:
    using value_type = T;

    template<typename U>
    struct rebind
    {
        using other = HeapArenaAllocator<U>;
    };

    HeapArenaAllocator() noexcept : m_provider(static_cast<HeapArena*>(nullptr)) {}

    HeapArenaAllocator(HeapArena* memProvider) noexcept
        : m_provider(memProvider) {}

    template<typename U>
    HeapArenaAllocator(const HeapArenaAllocator<U>& other) noexcept
        : m_provider(other.m_provider) {}

    T* allocate(std::size_t n) {
        assert(m_provider && "HeapArenaAllocator requires a valid Arena pointer");
        void* ptr = m_provider->allocate(n * sizeof(T), alignof(T));
        if (!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        assert(m_provider && "HeapArenaAllocator requires a valid Arena pointer");
        m_provider->destruct(p);
    }

    template<typename U>
    bool operator==(const HeapArenaAllocator<U>& rhs) const noexcept {
        return m_provider == rhs.m_provider;
    }

    template<typename U>
    bool operator!=(const HeapArenaAllocator<U>& rhs) const noexcept {
        return !(*this == rhs);
    }

    HeapArena* getMemoryProvider() const noexcept { return m_provider; }


private:
    HeapArena* m_provider;

    template<typename U> friend class HeapArenaAllocator;
};

namespace entt
{
    template <typename T, typename U>
    class basic_registry;
    class entity;
}

using HeapArenaEnttRegistry = entt::basic_registry<
    entt::entity,
    HeapArenaAllocator<entt::entity>>;
#endif