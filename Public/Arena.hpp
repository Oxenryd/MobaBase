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

#include "ArenaAllocator.hpp"

#ifdef LOGGING
    #include "Log.hpp"
#endif

#ifndef HEAP_ARENA_MAX_PAGES
    #define HEAP_ARENA_MAX_PAGES 1024
#endif

struct ArenaPage
{
    size_t offset;
    uint32_t size;
    uint8_t dataStartOffset = 0;
    uint8_t free = 1;
    bool isFree() { return free == 1; }
    void occupy(uint8_t dataOffset) { 
        free = 0;
        dataStartOffset = dataOffset;
    }
    void release() { 
        free = 1;
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
    HeapArena(size_t size)
        : m_size(size + 1), m_lastStart(0) {
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
            throw std::bad_alloc();
        }

        size_t current = reinterpret_cast<size_t>(m_memory);
        size_t alignment = alignof(std::max_align_t);
        size_t aligned = alignUp(current, alignment);
        size_t offset = aligned - reinterpret_cast<size_t>(m_memory);
        void* ptr = m_memory + offset;
        m_pagesPtr = new (ptr) std::vector<ArenaPage>();
        m_pagesPtr->reserve(HEAP_ARENA_MAX_PAGES);

        reset();

        m_destructorsPtr = constructNoRegister<std::vector<std::vector<DestructorEntry, HeapArenaAllocator<DestructorEntry>>>>();
        m_destructorsPtr->emplace_back(std::vector<DestructorEntry, HeapArenaAllocator<DestructorEntry>>{this});

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

    bool findFirstFreeSpot(uint32_t size, size_t& outOffset, size_t alignment = alignof(std::max_align_t)) {
        for (size_t i = 0; i < m_pagesPtr->size(); ++i) {
            auto& page = m_pagesPtr->at(i);
            if (!page.isFree())
                continue;

            auto aligned = alignUp(page.offset, alignment);
            auto delta = aligned - page.offset;
            auto actualSize = alignUp(size + delta, alignment);

            if (actualSize < page.size) {
                auto lastSize = page.size;
                page.size = actualSize;
                page.occupy(delta);
                outOffset = aligned;
                m_pagesPtr->emplace_back(ArenaPage{ page.offset + page.size, lastSize - size });
                mergePages();
                m_used += page.size;
                return true;
            }
        }
        return false;
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
            if (registerDestructor && m_destructorsPtr) {
                m_destructorsPtr->at(0).push_back({[](void* p) { static_cast<T*>(p)->~T(); }, obj});
            }
        }

        return obj;
    }

    inline void sortPages() {
        std::sort(m_pagesPtr->begin(), m_pagesPtr->end(),
                  [](const ArenaPage& a, const ArenaPage& b) {
                      return a.offset < b.offset;
                  });
    }

    inline void mergePages() {
        sortPages();
        size_t writeIndex = 0;
        for (size_t i = 1; i < m_pagesPtr->size(); ++i) {
            ArenaPage& prev = (*m_pagesPtr)[writeIndex];
            ArenaPage& curr = (*m_pagesPtr)[i];

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
        m_destructorsPtr->emplace_back();
        return id;
    }

    inline void slidePages(size_t fromIndex) {
        for (size_t i = fromIndex; i < m_pagesPtr->size() - 1; ++i) {
            m_pagesPtr->at(i) = m_pagesPtr->at(i + 1);
        }
    }

    template <typename T>
    inline void destruct(T* ptr) {
        size_t pageStart = reinterpret_cast<size_t>(ptr);
        ArenaPage* page = findPage(pageStart);
        if (!page)
            throw std::exception("No such pointer allocated.");
        static_cast<T*>(ptr)->~T();
        m_used -= page->size;
        page->release();
    }

    ArenaPage* findPage(size_t ptr) {
        auto mem = reinterpret_cast<size_t>(m_memory);
        for (auto& page : *m_pagesPtr) {
            if (mem + page.offset == ptr)
                return &page;
        }
        return nullptr;
    }

    void reset() {
        m_pagesPtr->clear();
        m_nextArenaId = 1;
        if (m_size > 0xffffffff) {
            size_t pageStart = sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES + sizeof(std::vector<ArenaPage>);
            uint32_t firstEnd = 0xffffffff - pageStart;
            m_pagesPtr->emplace_back(ArenaPage{ pageStart, firstEnd });
            m_pagesPtr->emplace_back(ArenaPage{ firstEnd + 1,  static_cast<uint32_t>(m_size - firstEnd + 1) });
        } else {
            size_t pageStart = sizeof(ArenaPage) * HEAP_ARENA_MAX_PAGES + sizeof(std::vector<ArenaPage>);
            m_pagesPtr->emplace_back(ArenaPage{ pageStart, static_cast<uint32_t>(m_size - pageStart) });
        }
    }

    const size_t& used() const { return m_used; }
    __declspec(dllexport) inline float ratioUsed() const { return static_cast<float>(m_used) / static_cast<float>(m_size); }

    size_t capacity() const { return m_size; }

    void destroyAll() {
        LOGLINE(LogType::Info, LogMod::Memory, "Destroying HeapArena elements, Addr: " +
                std::to_string(reinterpret_cast<size_t>(m_memory)) + "... ");
        for (auto& list : *m_destructorsPtr) {
            for (auto& entry : list) {
                if (entry.object && entry.destroyFunc)
                    entry.destroyFunc(entry.object);
#ifdef LOGGING
                else
                    LOG(LogType::Warning, "\n\t\tDestructor missing. Skipping... ");
#endif
            }
        }
        for (size_t i = m_destructorsPtr->size() - 1; i > 1; --i) {
            m_destructorsPtr->erase(m_destructorsPtr->end());
        } 
        reset();
        LOG(LogType::Success, "Done.");
    }

private:
    std::vector<ArenaPage>* m_pagesPtr = nullptr;
    std::vector<std::vector<DestructorEntry, HeapArenaAllocator<DestructorEntry>>>* m_destructorsPtr = nullptr;
    uint8_t* m_memory = nullptr;
    size_t m_size;
    size_t m_lastStart;
    size_t m_lastSize;
    uint32_t m_nextArenaId = 1;
    size_t m_used = 0;

    static size_t alignUp(size_t addr, size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
};

class Arena
{ 
public:

    Arena(HeapArena* const memProvider, size_t size)
        : m_size(size), m_offset(0) {
        m_memory = new uint8_t[size];
        LOGLINE(LogType::Info, LogMod::Memory, "Creating Arena, Addr: " + std::to_string(reinterpret_cast<size_t>(m_memory)) +
                ", " + std::to_string(m_size / 1024) + "kB... ");
        m_arenaId = memProvider->registerArena();
        m_destructorsPtr = constructNoRegister<std::vector<DestructorEntry, HeapArenaAllocator<DestructorEntry>>>(HeapArenaAllocator<DestructorEntry>(memProvider));
        LOG(LogType::Success, "Done.");
    }

    ~Arena() {
        destroyAll();
        //delete m_destructorsPtr;
        delete[] m_memory;
        m_memory = nullptr;
    }

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        size_t current = reinterpret_cast<size_t>(m_memory) + m_offset;
        size_t aligned = alignUp(current, alignment);
        size_t offset = aligned - reinterpret_cast<size_t>(m_memory);

        if (offset + size > m_size) {
            throw std::bad_alloc();// FOR DEBUG
            //return nullptr;
        }

        void* ptr = m_memory + offset;
        m_offset = offset + size;
        return ptr;
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

        if (registerDestructor && m_destructorsPtr) {
            m_destructorsPtr->push_back({ [](void* p) { static_cast<T*>(p)->~T(); }, obj });
        }
        return obj;
    }

    void reset() {
        m_offset = 0;
    }

    size_t used() const { return m_offset; }
    size_t capacity() const { return m_size; }
    float ratioUsed() const { return static_cast<float>(m_offset) / static_cast<float>(m_size); }

    void destroyAll() {
        LOGLINE(LogType::Info, LogMod::Memory, "Destroying Arena elements, Addr: " +
                std::to_string(reinterpret_cast<size_t>(m_memory)) + "... ");
        for (auto& entry : *m_destructorsPtr) {
            entry.destroyFunc(entry.object);
        }
        m_destructorsPtr->clear();
        reset();
        LOG(LogType::Success, "Done.");
    }

private:
    std::vector<DestructorEntry, HeapArenaAllocator<DestructorEntry>>* m_destructorsPtr = nullptr;
    uint8_t* m_memory = nullptr;
    size_t m_size;
    size_t m_offset;
    uint32_t m_arenaId;

    static size_t alignUp(size_t addr, size_t alignment) {
        return (addr + (alignment - 1)) & ~(alignment - 1);
    }
};


#endif