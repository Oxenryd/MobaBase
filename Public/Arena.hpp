#ifndef ARENA_HPP
#define ARENA_HPP

#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>
#include <algorithm>
#include <vector>
#include <semaphore>
#include <format>
#
#include "API.h"

#ifndef GLOBAL_MACROS_H
    #include "GlobalMacros.h"
#endif

#include "Delegate.hpp"
#include "Log.hpp"

using MemoryAddressDelta = int64_t;
using NewHeapBaseAddress = size_t;

#ifndef HEAP_ARENA_MAX_PAGES
    #define HEAP_ARENA_MAX_PAGES 2048
#endif

#ifdef _MSC_VER
#include <malloc.h>
inline void* aligned_alloc_compat(std::size_t alignment, std::size_t size) {
    return _aligned_malloc(size, alignment);
}
inline void aligned_free_compat(void* p) {
    _aligned_free(p);
}
#else
#include <cstdlib>
inline void* aligned_alloc_compat(std::size_t alignment, std::size_t size) {
    // C11/C++17 rule: size must be a multiple of alignment
    size = (size + alignment - 1) / alignment * alignment;
    return std::aligned_alloc(alignment, size);
}
inline void aligned_free_compat(void* p) {
    std::free(p);
}
#endif

struct ArenaPage
{
    size_t offset = 0;
    uint32_t size = 0;
    uint8_t dataStartOffset = 0;
    uint8_t free = 1;
    //uint16_t type = static_cast<uint16_t>(-1); // for future reflection
    [[nodiscard]] bool isFree() const { return free == 1; }
    void occupy(const uint8_t dataOffset /*, uint16_t typeIndex*/) {
        free = 0;
        dataStartOffset = dataOffset;
        //type = typeIndex;
    }
    void release() { 
        free = 1;
        //type = static_cast<uint16_t>(-1);
    }
};

struct DestructorEntry
{
    void (*destroyFunc)(void*);
    void* object;
};

class HeapArena
{
public:
    HeapArena() = delete;
    explicit HeapArena(const size_t size, const bool destruct = true)
        : m_size(size + 1), m_lastStart(0), m_destruct{ destruct }
    {
        constexpr size_t alignment = 64;
        m_memory = static_cast<uint8_t*>(aligned_alloc_compat(alignment, m_size));
        //m_memory = new uint8_t[m_size];
        const auto memStart = reinterpret_cast<size_t>(m_memory);
        if (memStart == 0) {
            LOGLINE(LogType::Info, LogMod::Memory, "Creating HeapArena, Addr: " + std::to_string(memStart) +
                    ", " + std::to_string(m_size / 1024) + "kB... ");
            LOG(LogType::Error, "Fail. Out of memory.");
            throw std::bad_alloc();
        }
      
        LOGLINE(LogType::Info, LogMod::Memory, "Creating HeapArena, Addr: " + std::to_string(memStart) +
                ", " + std::to_string(m_size / 1024) + "kB... ");

        if (size < sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES * 2) {
            LOG(LogType::Error, "Fail. Size too small.");
            aligned_free_compat(m_memory);
            throw std::bad_alloc();
        }

        m_pagesPtr.reserve(HEAP_ARENA_MAX_PAGES);

        reset();

        if (m_destruct)
            m_destructorsPtr.emplace_back();
        
        m_lastSize = size;
        LOG(LogType::Success, "Done.");
    }

    ~HeapArena() {
        destroyAll();
        aligned_free_compat(m_memory);
    }

    void* allocate(const size_t size, const size_t alignment = alignof(std::max_align_t)) {
        
        size_t dataOffset;
        if (!findFirstFreeSpot(size, dataOffset, alignment)) {
            return nullptr;
        }

        void* ptr = m_memory + dataOffset;
        return ptr;
    }

    bool findFirstFreeSpot(const size_t size, size_t& outOffset, const size_t alignment = alignof(std::max_align_t)) {
        for (size_t i = 0; i < m_pagesPtr.size(); ++i) {
            auto& page = m_pagesPtr[i];
            if (!page.isFree())
                continue;

            const auto aligned = alignUp(page.offset, alignment);
            const auto delta = aligned - page.offset;
            const auto actualSize = alignUp(size + delta, alignment);

            if (actualSize < page.size) {
                const size_t lastSize = page.size;
                page.size = static_cast<uint32_t>(actualSize);
                page.occupy(static_cast<uint8_t>(delta));
                outOffset = aligned;
                m_pagesPtr.emplace_back(ArenaPage{ page.offset + page.size, static_cast<uint32_t>(lastSize - size )});
                mergePages();
                m_used += page.size;
                return true;
            }
        }
        return false;
    }

    void resize(const size_t newSizeBytes) {
        if (newSizeBytes <= m_size)
            return;

        auto* temp = new uint8_t[newSizeBytes];       
        std::memcpy(temp, m_memory, m_size);

        const auto delta =
            static_cast<MemoryAddressDelta>(reinterpret_cast<size_t>(temp) - reinterpret_cast<size_t>(m_memory));
        reallocated.notify(delta);
        aligned_free_compat(m_memory);
        m_size = newSizeBytes;
        if (m_pagesPtr.back().isFree()) {
            m_pagesPtr.back().size = static_cast<uint32_t>(m_size - m_pagesPtr.back().offset);
        }
        m_memory = temp;
    }

    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        return constructImpl<T>(true, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T* constructNoRegister(Args&&... args) {
        return constructImpl<T>(false, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T* constructImpl(const bool registerDestructor, Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        T* obj = new (mem) T(std::forward<Args>(args)...);

        if constexpr (std::is_same_v<T, Arena>) {

        } 
        else {
            if (registerDestructor) {
                m_destructorsPtr[0].push_back({
                    [](void* p) { 
                        static_cast<T*>(p)->~T();
                    }, obj});
            }
        }

        return obj;
    }

    void sortPages() {
        std::ranges::sort(m_pagesPtr.begin(), m_pagesPtr.end(),
                  [](const ArenaPage& a, const ArenaPage& b) {
                      return a.offset < b.offset;
                  });
    }

    void mergePages() {
        sortPages();
        size_t writeIndex = 0;
        for (size_t i = 1; i < m_pagesPtr.size(); ++i) {
            ArenaPage& prev = m_pagesPtr[writeIndex];
            ArenaPage& curr = m_pagesPtr[i];

            if (!curr.isFree()) {
                writeIndex++;
                continue;
            }
            
            // Merge
            if (prev.isFree()) {
                curr.size += prev.size;
                slidePages(i);
            }
            writeIndex++;
        }
    }

    uint32_t registerArena() {
        const auto id = m_nextArenaId++;
        if (m_destruct)
            m_destructorsPtr.emplace_back();
        return id;
    }

    void slidePages(const size_t fromIndex) {
        for (size_t i = fromIndex; i < m_pagesPtr.size() - 1; ++i) {
            m_pagesPtr[i] = m_pagesPtr[i + 1];
        }
    }

    template <typename T>
    void deallocate(T* ptr, size_t size) {
        destruct(ptr, size);
    }

    template <typename T>
    void destruct(T* ptr, const size_t size) {
        constexpr auto pageStart = reinterpret_cast<size_t>(ptr);
        ArenaPage* page = findPage(pageStart);
        if (!page)
            throw std::runtime_error("No such pointer allocated.");
        if (page->size != size)
            throw std::runtime_error("Deallocation/Page size mismatch.");
        if (m_destruct)
            static_cast<T*>(ptr)->~T();
        m_used -= page->size;
        page->release();
    }

    ArenaPage* findPage(const size_t ptr) {
        const auto mem = reinterpret_cast<size_t>(m_memory);
        for (auto& page : m_pagesPtr) {
            if (mem + page.offset == ptr)
                return &page;
        }
        return nullptr;
    }

    void reset() {
        m_pagesPtr.clear();
        m_nextArenaId = 1;
        if (m_size > 0xffffffff) {
            constexpr size_t pageStart = 0; //sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES + sizeof(std::vector<ArenaPage>);
            constexpr uint32_t firstEnd = 0xffffffff - static_cast<uint32_t>(pageStart);
            m_pagesPtr.emplace_back(ArenaPage{ pageStart, firstEnd });
            m_pagesPtr.emplace_back(ArenaPage{ firstEnd + 1,  static_cast<uint32_t>(m_size - firstEnd + 1) });
        } else {
            constexpr size_t pageStart = 0;//sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES + sizeof(std::vector<ArenaPage>);
            m_pagesPtr.emplace_back(ArenaPage{ pageStart, static_cast<uint32_t>(m_size - pageStart + 1) });
        }
    }

    [[nodiscard]] size_t used() const { return m_used; }
    ENGINE_API [[nodiscard]] float ratioUsed() const { return static_cast<float>(m_used) / static_cast<float>(m_size); }

    [[nodiscard]] size_t capacity() const { return m_size; }

    void destroyAll() {
        const auto address = reinterpret_cast<size_t>(m_memory);
        const std::string addrStr = std::to_string(address);

        LOGLINE_IND(LogType::Info, LogMod::Memory, "Destroying HeapArena elements, Addr: " +
                addrStr + "... ", 1);
        if (m_destruct)             {
            for (auto& list : m_destructorsPtr) {
                for (const auto&[destroyFunc, object] : list) {
                    if (object && destroyFunc) {
                        try {
                            destroyFunc(object);
                        } catch (std::exception& e) {
                            const auto what = std::string{e.what()};
                            const auto msg = std::string{"Failed to execute a destructor: " + what};
                            LOGLINE( LogType::Warning, LogMod::Memory, msg);
                        }
                    }
                    #ifdef LOGGING
                    else
                        LOG(LogType::Warning, "\n\t\tDestructor missing. Skipping... ");
                    #endif
                }
            }
            for (size_t i = m_destructorsPtr.size() - 1; i > 1; --i) {
                m_destructorsPtr.erase(m_destructorsPtr.end());
            }
        }

        m_pagesPtr.clear();
        m_pagesPtr.resize(0);
        reset();
        LOGLINE_IND(LogType::Success, LogMod::Memory, "HeapArena, Addr: " + addrStr + " deallocated.", -1);
    }

private:
    std::vector<ArenaPage> m_pagesPtr;
    std::vector<std::vector<DestructorEntry>> m_destructorsPtr;
    Event<MemoryAddressDelta> reallocated;
    uint8_t* m_memory = nullptr;
    size_t m_size;
    [[maybe_unused]] size_t m_lastStart;
    [[maybe_unused]] size_t m_lastSize;
    uint32_t m_nextArenaId = 1;
    size_t m_used = 0;
    bool m_destruct = true;

    static size_t alignUp(const size_t addr, const size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
};





class Arena
{
    HeapArena* m_heap = nullptr;
    uint8_t* m_memory = nullptr;
    size_t m_size;
    size_t m_offset;
    uint32_t m_arenaId;
    std::binary_semaphore m_sem{ 1 };

    static size_t alignUp(const size_t addr, const size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
public:

    explicit Arena(const size_t size, HeapArena* const memProvider = nullptr)
        : m_size(size), m_offset(0) {
        
        m_heap = memProvider;
        //m_memory = memProvider ? reinterpret_cast<uint8_t*>(m_heap->allocate(size)) : new uint8_t[size];
        m_memory = memProvider 
            ? static_cast<uint8_t*>(m_heap->allocate(size))
            : static_cast<uint8_t*>(operator new[](size, std::align_val_t{64}));
        m_arenaId = memProvider ? m_heap->registerArena() : UINT32_INVALID;
        LOGLINE(LogType::Info, LogMod::Memory, "Creating Arena, Addr: " + std::to_string(reinterpret_cast<size_t>(m_memory)) +
                ", " + std::to_string(m_size / 1024) + "kB... ");
        LOG(LogType::Success, "Done.");
    }
    Arena(const Arena& other) :
        m_heap{other.m_heap},
        m_memory{other.m_memory},
        m_size{other.m_size},
        m_offset{other.m_offset},
        m_arenaId{other.m_arenaId} {}

    Arena(Arena&& other) noexcept {
        m_heap = other.m_heap;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_offset = other.m_offset;
        m_arenaId = other.m_arenaId;

        other.m_heap = nullptr;
        other.m_memory = nullptr;
        other.m_size = 0;
        other.m_arenaId = UINT32_INVALID;
    }

    Arena& operator=(Arena&& rhs) noexcept {
        m_heap = rhs.m_heap;
        m_memory = rhs.m_memory;
        m_size = rhs.m_size;
        m_offset = rhs.m_offset;
        m_arenaId = rhs.m_arenaId;

        rhs.m_heap = nullptr;
        rhs.m_memory = nullptr;
        rhs.m_size = 0;
        rhs.m_arenaId = UINT32_INVALID;
        return *this;
    }

    ~Arena() {
        destroyAll();
        //delete[] m_memory;
        aligned_free_compat(m_memory);
        //operator delete[](m_memory, std::align_val_t{ 64 });
        m_memory = nullptr;
    }

    void* allocate(const size_t size, const size_t alignment = alignof(std::max_align_t)) {
        m_sem.acquire();
        const size_t current = reinterpret_cast<size_t>(m_memory) + m_offset;
        const size_t aligned = alignUp(current, alignment);
        const size_t offset = aligned - reinterpret_cast<size_t>(m_memory);

        if (offset + size > m_size) {
            throw std::bad_alloc();// FOR DEBUG
            //return nullptr;
        }

        void* ptr = m_memory + offset;
        m_offset = offset + size;
        m_sem.release();
        return ptr;
    }

    static void* destruct(void*, size_t) { return nullptr; }
    static void deallocate(void*, size_t) {}


    //template<typename T, typename... Args>
    //T* construct(Args&&... args) {
    //    return constructImpl<T>(true, std::forward<Args>(args)...);
    //}

    //template<typename T, typename... Args>
    //T* constructNoRegister(Args&&... args) {
    //    return constructImpl<T>(false, std::forward<Args>(args)...);
    //}

    //template<typename T, typename... Args>
    //T* constructImpl(bool registerDestructor, Args&&... args) {
    //    void* mem = allocate(sizeof(T), alignof(T));
    //    T* obj = new (mem) T(std::forward<Args>(args)...);

    //    if (registerDestructor && m_destructorsPtr) {
    //        m_destructorsPtr->push_back({ [](void* p) { static_cast<T*>(p)->~T(); }, obj });
    //    }
    //    return obj;
    //}

    void reset() {
        m_sem.acquire();
        m_offset = 0;
        m_sem.release();
    }

    [[nodiscard]] size_t used() const { return m_offset; }
    [[nodiscard]] size_t capacity() const { return m_size; }
    [[nodiscard]] float ratioUsed() const { return static_cast<float>(m_offset) / static_cast<float>(m_size); }

    void destroyAll() {
        LOGLINE(LogType::Info, LogMod::Memory, "Destroying Arena elements, Addr: " +
                std::to_string(reinterpret_cast<size_t>(m_memory)) + "... ");
        //for (auto& entry : *m_destructorsPtr) {
        //    entry.destroyFunc(entry.object);
        //}
        //m_destructorsPtr->clear();
        reset();
        LOG(LogType::Success, "Done.");
    }
};

class FrameArena
{
    uint8_t* m_memory;
    size_t m_size;
    size_t m_offset{ 0 }; // Thread-safe bump pointer

public:
    explicit FrameArena(const size_t size) : m_size(size) {
        m_memory = static_cast<uint8_t*>(operator new[](size, std::align_val_t{ 64 }));//new uint8_t[size];
        if (!m_memory) throw std::bad_alloc();
    }

    ~FrameArena() {
        aligned_free_compat(m_memory);
        //operator delete[](m_memory, std::align_val_t{ 64 });
    }

    void* allocate(const size_t size, const size_t alignment = alignof(std::max_align_t)) {
        const size_t current = m_offset; // .load(std::memory_order_relaxed);

        //size_t aligned, newOffset;
        //do {
            //aligned = alignUp(reinterpret_cast<uintptr_t>(m_memory + current), alignment)
            //    - reinterpret_cast<uintptr_t>(m_memory);
            const size_t aligned = alignUp(current, alignment);
            const size_t newOffset = aligned + size;

            if (newOffset > m_size) {
                throw std::bad_alloc();
                //return nullptr; // Out of memory
            }
            m_offset = newOffset;
        //} while (!m_offset.compare_exchange_weak(current, newOffset, std::memory_order_relaxed));

        return m_memory + aligned;
    }

    // No individual deallocate - reset entire frame
    static void deallocate(void*, size_t) {
        // No-op for frame allocator
    }

    void reset() {
        m_offset = 0; // .store(0, std::memory_order_release);
    }

    [[nodiscard]] size_t used() const {
        return m_offset;//.load(std::memory_order_relaxed);
    }

    [[nodiscard]] float ratioUsed() const {
        return static_cast<float>(used()) / static_cast<float>(m_size);
    }

private:
    static size_t alignUp(const uintptr_t addr, const size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
};

#endif