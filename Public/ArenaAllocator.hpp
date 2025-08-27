#ifndef ARENAALLOCATOR_HPP
#define ARENAALLOCATOR_HPP

#include <cstdint>
#include <entt/entt.hpp>

class Arena;
class HeapArena;

#ifndef ARENA_HPP
    #include "Arena.hpp"
#endif

template<typename T>
class HeapArenaAllocator
{
private:
    HeapArena* m_provider;
    template<typename U> friend class HeapArenaAllocator;
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
};

using HeapArenaRegistry = entt::basic_registry<entt::entity, HeapArenaAllocator<entt::entity>>;

template<typename T>
class ArenaAllocator
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::false_type;

    template<typename U>
    struct rebind
    {
        using other = ArenaAllocator<U>;
    };

    ArenaAllocator() noexcept : m_provider(static_cast<Arena*>(nullptr)) {}


    ArenaAllocator(Arena* const memProvider) noexcept
        : m_provider(memProvider) {}

    template<typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) noexcept
        : m_provider(other.m_provider) {}

    // Add copy constructor and assignment operator
    ArenaAllocator(const ArenaAllocator&) = default;
    ArenaAllocator& operator=(const ArenaAllocator&) = default;

    // Add move constructor and assignment operator
    ArenaAllocator(ArenaAllocator&&) = default;
    ArenaAllocator& operator=(ArenaAllocator&&) = default;

    ~ArenaAllocator() = default;

    T* allocate(std::size_t n) {
        assert(m_provider && "ArenaAllocator requires a valid Arena pointer");
        void* ptr = m_provider->allocate(n * sizeof(T), alignof(T));
        if (!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        assert(m_provider && "ArenaAllocator requires a valid Arena pointer");
        m_provider->deallocate(p, n * sizeof(T)); // Adjust based on your HeapArena interface
    }

    // Add construct and destroy methods (optional but can help)
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        p->~U();
    }

    template<typename U>
    bool operator==(const ArenaAllocator<U>& rhs) const noexcept {
        return m_provider == rhs.m_provider;
    }

    template<typename U>
    bool operator!=(const ArenaAllocator<U>& rhs) const noexcept {
        return !(*this == rhs);
    }

    Arena* getMemoryProvider() const noexcept { return m_provider; }

private:
    Arena* m_provider;
    template<typename U> friend class ArenaAllocator;
};

using ArenaRegistry = entt::basic_registry<entt::entity, ArenaAllocator<entt::entity>>;

template <typename T>
using ArenaVector = std::vector<T, ArenaAllocator<T>>;

template <typename Key, typename Value>
using ArenaUMap = std::unordered_map<
    Key,
    Value,
    std::hash<Key>,
    std::equal_to<Key>,
    ArenaAllocator<std::pair<const Key, Value>>
>;









template<typename T>
using ArenaStorage = entt::basic_storage<entt::entity, T, ArenaAllocator<T>>;

template<typename T>
struct StorageSelector
{
    using type = ArenaStorage<T>;
};


//namespace entt
//{
//    template<>
//    struct storage_type<TransformComponent, ArenaRegistry>
//    {
//        using type = ArenaStorage<TransformComponent>;
//    };
//
//    template<>
//    struct storage_type<EnabledTag, ArenaRegistry>
//    {
//        using type = ArenaStorage<EnabledTag>;
//    };
//
//    template<>
//    struct storage_type<TagComponent, ArenaRegistry>
//    {
//        using type = ArenaStorage<TagComponent>;
//    };
//
//    // Repeat as needed for each component type
//}




#endif