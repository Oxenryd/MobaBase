#ifndef ARENA_HPP
#define ARENA_HPP

#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>
#include <algorithm>
#include <memory>
#include <vector>
#include <semaphore>


#ifndef GLOBAL_MACROS_H
    #include "GlobalMacros.h"
#endif

#include "Delegate.hpp"
#include "ArenaAllocator.hpp"
#include "Log.hpp"

using MemoryAddressDelta = int64_t;
using NewHeapBaseAddress = size_t;

#ifndef HEAP_ARENA_MAX_PAGES
    #define HEAP_ARENA_MAX_PAGES 2048
#endif

struct ArenaPage
{
    size_t offset = 0;
    uint32_t size = 0;
    uint8_t dataStartOffset = 0;
    uint8_t free = 1;
    //uint16_t type = static_cast<uint16_t>(-1); // for future reflection
    bool isFree() { return free == 1; }
    void occupy(uint8_t dataOffset /*, uint16_t typeIndex*/) {
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
    HeapArena(size_t size, bool destruct = true)
        : m_size(size + 1), m_lastStart(0), m_destruct{ destruct }
    {
        m_memory = new uint8_t[m_size];
        size_t memStart = reinterpret_cast<size_t>(m_memory);
        if (!m_memory) {
            LOGLINE(LogType::Info, LogMod::Memory, "Creating HeapArena, Addr: " + std::to_string(memStart) +
                    ", " + std::to_string(m_size / 1024) + "kB... ");
            LOG(LogType::Error, "Fail. Out of memory.");
            throw std::bad_alloc();
        }
      
        LOGLINE(LogType::Info, LogMod::Memory, "Creating HeapArena, Addr: " + std::to_string(memStart) +
                ", " + std::to_string(m_size / 1024) + "kB... ");

        if (size < sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES * 2) {
            LOG(LogType::Error, "Fail. Size too small.");
            delete[] m_memory;
            throw std::bad_alloc();
        }

        m_pagesPtr.reserve(HEAP_ARENA_MAX_PAGES);

        reset();

        if (m_destruct)
            m_destructorsPtr.emplace_back(std::vector<DestructorEntry>{});
        

        LOG(LogType::Success, "Done.");
    }

    ~HeapArena() {
        destroyAll();
        delete[] m_memory;
    }

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        
        size_t dataOffset;
        if (!findFirstFreeSpot(size, dataOffset, alignment)) {
            return nullptr;
        }

        void* ptr = m_memory + dataOffset;
        return ptr;
    }

    bool findFirstFreeSpot(size_t size, size_t& outOffset, size_t alignment = alignof(std::max_align_t)) {
        for (size_t i = 0; i < m_pagesPtr.size(); ++i) {
            auto& page = m_pagesPtr[i];
            if (!page.isFree())
                continue;

            auto aligned = alignUp(page.offset, alignment);
            auto delta = aligned - page.offset;
            auto actualSize = alignUp(size + delta, alignment);

            if (actualSize < page.size) {
                size_t lastSize = page.size;
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

    void resize(size_t newSizeBytes) {
        if (newSizeBytes <= m_size)
            return;

        auto* temp = new uint8_t[newSizeBytes];       
        std::memcpy(temp, m_memory, m_size);

        MemoryAddressDelta delta = reinterpret_cast<size_t>(temp) - reinterpret_cast<size_t>(m_memory);
        reallocated.notify(delta);
        delete m_memory;
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
    T* constructImpl(bool registerDestructor, Args&&... args) {
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

    inline void sortPages() {
        std::sort(m_pagesPtr.begin(), m_pagesPtr.end(),
                  [](const ArenaPage& a, const ArenaPage& b) {
                      return a.offset < b.offset;
                  });
    }

    inline void mergePages() {
        sortPages();
        size_t writeIndex = 0;
        for (size_t i = 1; i < m_pagesPtr.size(); ++i) {
            ArenaPage& prev = (m_pagesPtr)[writeIndex];
            ArenaPage& curr = (m_pagesPtr)[i];

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
        auto id = m_nextArenaId++;
        if (m_destruct)
            m_destructorsPtr.emplace_back();
        return id;
    }

    inline void slidePages(size_t fromIndex) {
        for (size_t i = fromIndex; i < m_pagesPtr.size() - 1; ++i) {
            m_pagesPtr[i] = m_pagesPtr[i + 1];
        }
    }

    template <typename T>
    inline void deallocate(T* ptr, size_t size) {
        destruct(ptr, size);
    }

    template <typename T>
    inline void destruct(T* ptr, size_t size) {
        size_t pageStart = reinterpret_cast<size_t>(ptr);
        ArenaPage* page = findPage(pageStart);
        if (!page)
            throw std::exception("No such pointer allocated.");
        if (page->size != size)
            throw std::exception("Deallocation/Page size mismatch.");
        if (m_destruct)
            static_cast<T*>(ptr)->~T();
        m_used -= page->size;
        page->release();
    }

    ArenaPage* findPage(size_t ptr) {
        auto mem = reinterpret_cast<size_t>(m_memory);
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
            size_t pageStart = 0;//sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES + sizeof(std::vector<ArenaPage>);
            uint32_t firstEnd = 0xffffffff - static_cast<uint32_t>(pageStart);
            m_pagesPtr.emplace_back(ArenaPage{ pageStart, firstEnd });
            m_pagesPtr.emplace_back(ArenaPage{ firstEnd + 1,  static_cast<uint32_t>(m_size - firstEnd + 1) });
        } else {
            size_t pageStart = 0;//sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES + sizeof(std::vector<ArenaPage>);
            m_pagesPtr.emplace_back(ArenaPage{ pageStart, static_cast<uint32_t>(m_size - pageStart + 1) });
        }
    }

    const size_t& used() const { return m_used; }
    __declspec(dllexport) inline float ratioUsed() const { return static_cast<float>(m_used) / static_cast<float>(m_size); }

    size_t capacity() const { return m_size; }

    void destroyAll() {
        size_t address = reinterpret_cast<size_t>(m_memory);
        std::string addrStr = std::to_string(address);

        LOGLINE_IND(LogType::Info, LogMod::Memory, "Destroying HeapArena elements, Addr: " +
                addrStr + "... ", 1);
        if (m_destruct)             {
            for (auto& list : m_destructorsPtr) {
                for (auto& entry : list) {
                    if (entry.object && entry.destroyFunc) {
                        try {
                            entry.destroyFunc(entry.object);
                        } catch (std::exception& e) {
                            LOGLINE(LogType::Warning, LogMod::Memory, std::format("Failed to execute a destructor: {}" ,e.what()));
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
    size_t m_lastStart;
    size_t m_lastSize;
    uint32_t m_nextArenaId = 1;
    size_t m_used = 0;
    bool m_destruct = true;

    static size_t alignUp(size_t addr, size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
};





class Arena
{ 
private:
    HeapArena* m_heap = nullptr;
    uint8_t* m_memory = nullptr;
    size_t m_size;
    size_t m_offset;
    uint32_t m_arenaId;
    std::binary_semaphore m_sem{ 1 };

    static size_t alignUp(size_t addr, size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
public:

    Arena(size_t size, HeapArena* const memProvider = nullptr)
        : m_size(size), m_offset(0) {
        
        m_heap = memProvider;
        //m_memory = memProvider ? reinterpret_cast<uint8_t*>(m_heap->allocate(size)) : new uint8_t[size];
        m_memory = memProvider 
            ? reinterpret_cast<uint8_t*>(m_heap->allocate(size))
            : reinterpret_cast<uint8_t*>(::operator new[](size, std::align_val_t{64}));
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
        ::operator delete[](m_memory, std::align_val_t{ 64 });
        m_memory = nullptr;
    }

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        m_sem.acquire();
        size_t current = reinterpret_cast<size_t>(m_memory) + m_offset;
        size_t aligned = alignUp(current, alignment);
        size_t offset = aligned - reinterpret_cast<size_t>(m_memory);

        if (offset + size > m_size) {
            throw std::bad_alloc();// FOR DEBUG
            //return nullptr;
        }

        void* ptr = m_memory + offset;
        m_offset = offset + size;
        m_sem.release();
        return ptr;
    }

    void* destruct(void* ptr, size_t size) {}
    void deallocate(void* ptr, size_t size) {}


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

    size_t used() const { return m_offset; }
    size_t capacity() const { return m_size; }
    float ratioUsed() const { return static_cast<float>(m_offset) / static_cast<float>(m_size); }

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
private:
    uint8_t* m_memory;
    size_t m_size;
    size_t m_offset{ 0 }; // Thread-safe bump pointer

public:
    FrameArena(size_t size) : m_size(size) {
        m_memory = reinterpret_cast<uint8_t*>(::operator new[](size, std::align_val_t{ 64 }));//new uint8_t[size];
        if (!m_memory) throw std::bad_alloc();
    }

    ~FrameArena() {
        ::operator delete[](m_memory, std::align_val_t{ 64 });
    }

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        size_t current = m_offset; // .load(std::memory_order_relaxed);
        size_t aligned, newOffset;

        //do {
            //aligned = alignUp(reinterpret_cast<uintptr_t>(m_memory + current), alignment)
            //    - reinterpret_cast<uintptr_t>(m_memory);
            aligned = alignUp(current, alignment);
            newOffset = aligned + size;

            if (newOffset > m_size) {
                throw std::bad_alloc();
                return nullptr; // Out of memory
            }
            m_offset = newOffset;
        //} while (!m_offset.compare_exchange_weak(current, newOffset, std::memory_order_relaxed));

        return m_memory + aligned;
    }

    // No individual deallocate - reset entire frame
    void deallocate(void*, size_t) {
        // No-op for frame allocator
    }

    void reset() {
        m_offset = 0;// .store(0, std::memory_order_release);
    }

    size_t used() const {
        return m_offset;//.load(std::memory_order_relaxed);
    }

    float ratioUsed() const {
        return static_cast<float>(used()) / static_cast<float>(m_size);
    }

private:
    static size_t alignUp(uintptr_t addr, size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
};

#endif