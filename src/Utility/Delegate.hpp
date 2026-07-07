#ifndef DELEGATE_HPP
#define DELEGATE_HPP

#include <cstdint>
#include <utility>
#include <type_traits>
#include <cstring>
#include <vector>

// 64 bytes SBO (can be tuned)
constexpr size_t SBO_SIZE = 64;

template<typename... Args>
class Delegate
{
private:
    using InvokeFn = void(*)(void* storage, Args...);
    using DestroyFn = void(*)(void* storage);
    using CopyFn = void(*)(void* dest, const void* src);

    void* m_storage = nullptr;
    InvokeFn m_invoke = nullptr;
    DestroyFn m_destroy = nullptr;
    CopyFn m_copy = nullptr;
    bool m_owns = false;
    bool m_usingSBO = false;

    uint8_t m_sbo[SBO_SIZE];

public:
    Delegate() = default;
    ~Delegate() { reset(); }

    Delegate(const Delegate& other) { copyFrom(other); }
    Delegate& operator=(const Delegate& other) {
        if (this != &other) {
            reset();
            copyFrom(other);
        }
        return *this;
    }
    template<typename F>
    Delegate(F&& func) {
        bind(std::forward<F>(func));
    }
    Delegate(Delegate&& other) noexcept { moveFrom(std::move(other)); }
    Delegate& operator=(Delegate&& other) noexcept {
        if (this != &other) {
            reset();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void reset() {
        if (m_destroy && m_owns) {
            m_destroy(storage());
        }

        if (m_owns && !m_usingSBO) {
            operator delete(storage()); // Safe generic delete
        }

        m_invoke = nullptr;
        m_destroy = nullptr;
        m_copy = nullptr;
        m_storage = nullptr;
        m_owns = false;
        m_usingSBO = false;
    }

    template<typename F>
    void bind(F&& func) {
        using Functor = std::decay_t<F>;
        reset();

        if constexpr (sizeof(Functor) <= SBO_SIZE && alignof(Functor) <= alignof(std::max_align_t)) {
            new (m_sbo) Functor(std::forward<F>(func));
            m_storage = m_sbo;
            m_usingSBO = true;
        } else {
            m_storage = new Functor(std::forward<F>(func));
            m_usingSBO = false;
        }

        m_owns = true;

        m_invoke = [](void* storage, Args... args) {
            auto* f = reinterpret_cast<Functor*>(storage);
            (*f)(args...);
            };

        m_destroy = [](void* storage) {
            auto* f = reinterpret_cast<Functor*>(storage);
            f->~Functor();
            };
    }

    void operator()(Args... args) const {
        if (m_invoke)
            m_invoke(storage(), args...);
    }

    explicit operator bool() const { return m_invoke != nullptr; }

private:
    void* storage() const {
        return (m_storage == m_sbo) ? const_cast<uint8_t*>(m_sbo) : m_storage;
    }

    void copyFrom(const Delegate& other) {
        m_invoke = other.m_invoke;
        m_destroy = other.m_destroy;
        m_copy = other.m_copy;
        m_owns = other.m_owns;

        if (m_owns) {
            if (other.m_storage == other.m_sbo) {
                m_copy(m_sbo, other.m_sbo);
                m_storage = m_sbo;
            } else {
                m_storage = operator new(SBO_SIZE); // allocate heap copy
                m_copy(m_storage, other.m_storage);
            }
        }
    }

    void moveFrom(Delegate&& other) {
        m_invoke = other.m_invoke;
        m_destroy = other.m_destroy;
        m_copy = other.m_copy;
        m_storage = other.m_storage;
        m_owns = other.m_owns;

        if (other.m_storage == other.m_sbo)
            std::memcpy(m_sbo, other.m_sbo, SBO_SIZE);

        if (other.m_storage == other.m_sbo)
            m_storage = m_sbo;

        other.m_storage = nullptr;
        other.m_invoke = nullptr;
        other.m_destroy = nullptr;
        other.m_copy = nullptr;
        other.m_owns = false;
    }
};



template<typename... Args>
class Event
{
    std::vector<Delegate<Args...>> m_listeners;

public:
    template<typename F>
    void subscribe(F&& f) {
        m_listeners.emplace_back(std::forward<F>(f));
    }

    void notify(Args... args) const {
        for (const auto& cb : m_listeners)
            cb(args...);
    }

    void unsubscribeAll() {
        m_listeners.clear();
    }
};

#endif