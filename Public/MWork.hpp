#ifndef MWORK_HPP
#define MWORK_HPP

#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>
#include <type_traits>
#include <new>
#include <cassert>
#include <limits>
#include <semaphore>
#include <algorithm>

#include "ArenaAllocator.hpp"

// ----------------- Arena adapter expectations -----------------
// Your Arena should expose:
//   void* allocate(std::size_t bytes, std::size_t align);
//   void  deallocate(void* p, std::size_t bytes, std::size_t align);
// If your interface differs, wrap it in a small adapter with those two methods.

namespace MWork
{

    // ----------------- Await points -----------------
    enum class JobAwaitPoint : uint16_t
    {
        Immediate,
        PreDraw,
        EndFrame,
        StartNextFrame,
        Count
    };

    // ----------------- Callable payload (arena-backed, no std::function) -----------------
    struct CallablePayload
    {
        void (*invoke)(void* obj, std::size_t shardIdx, std::size_t shardCount, std::size_t threadIndex) = nullptr;
        void (*destroy)(void* obj, void* arena) = nullptr;
        void* obj = nullptr;
        void* arena = nullptr;
    };

    template <class Fn>
    struct CallableOps
    {
        static void call(void* p, std::size_t shardIdx, std::size_t shardCount, std::size_t threadIndex) {
            (*static_cast<Fn*>(p))(shardIdx, shardCount, threadIndex);
        }
        static void dtor(void* p, void* a) {
            auto* fn = static_cast<Fn*>(p);
            auto* ar = static_cast<FrameArena*>(a);
            fn->~Fn();
            ar->deallocate(fn, sizeof(Fn));
        }
    };

    template <class Fn>
    static inline CallablePayload make_callable(Fn&& f, FrameArena* arena) {
        using F = std::decay_t<Fn>;
        void* mem = arena->allocate(sizeof(F), alignof(F));
        F* obj = new (mem) F(std::forward<Fn>(f));
        CallablePayload p;
        p.invoke = &CallableOps<F>::call;
        p.destroy = &CallableOps<F>::dtor;
        p.obj = obj;
        p.arena = arena;
        return p;
    }

    // ----------------- Job group (fan-out/fan-in) -----------------
    struct JobGroup
    {
        std::atomic<uint32_t> pending{ 0 };
        std::binary_semaphore done{ 0 };   // released once when pending hits zero
        CallablePayload payload{};       // shared callable for all shards in the group
        JobAwaitPoint awaitPoint{ JobAwaitPoint::Immediate };

        explicit JobGroup(uint32_t count, JobAwaitPoint ap, CallablePayload payload_)
            : pending(count), done(0), payload(payload_), awaitPoint(ap) {}
        JobGroup(const JobGroup&) = delete;
        JobGroup& operator=(const JobGroup&) = delete;
    };

    // Lightweight handle the caller can wait on (or engine can stash by await point)
    using JobHandle = JobGroup*;

    inline void wait(JobHandle h) {
        if (!h) return;
        h->done.acquire(); // one-shot
        // destroy callable & free group storage happen in worker that observed pending==0
    }

    // ----------------- Bounded MPMC ring queue (Vyukov-style) -----------------
    template <class T>
    class MpmcQueue
    {
    public:
        explicit MpmcQueue(std::size_t capacityPow2)
            : m_mask(capacityPow2 - 1),
            m_cells(capacityPow2) {
            assert(capacityPow2 && (capacityPow2 & (capacityPow2 - 1)) == 0 && "capacity must be power of two");
            for (std::size_t i = 0; i < capacityPow2; ++i) {
                m_cells[i].seq.store(i, std::memory_order_relaxed);
            }
            m_head.store(0, std::memory_order_relaxed);
            m_tail.store(0, std::memory_order_relaxed);
        }

        // Unbounded retries; for hot path you usually size big enough to avoid spinning.
        bool enqueue(const T& data) {
            Cell* cell;
            std::size_t pos = m_tail.load(std::memory_order_relaxed);
            for (;;) {
                cell = &m_cells[pos & m_mask];
                std::size_t seq = cell->seq.load(std::memory_order_acquire);
                intptr_t dif = (intptr_t)seq - (intptr_t)pos;
                if (dif == 0) {
                    if (m_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                        break;
                } else if (dif < 0) {
                    return false; // full
                } else {
                    pos = m_tail.load(std::memory_order_relaxed);
                }
            }
            cell->data = data;
            cell->seq.store(pos + 1, std::memory_order_release);
            return true;
        }

        bool dequeue(T& data) {
            Cell* cell;
            std::size_t pos = m_head.load(std::memory_order_relaxed);
            for (;;) {
                cell = &m_cells[pos & m_mask];
                std::size_t seq = cell->seq.load(std::memory_order_acquire);
                intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
                if (dif == 0) {
                    if (m_head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                        break;
                } else if (dif < 0) {
                    return false; // empty
                } else {
                    pos = m_head.load(std::memory_order_relaxed);
                }
            }
            data = cell->data;
            cell->seq.store(pos + m_mask + 1, std::memory_order_release);//cell->seq.store(pos + m_mask + 1 + 1, std::memory_order_release);
            return true;
        }

    private:
        struct Cell
        {
            std::atomic<std::size_t> seq;
            T data;
        };

        const std::size_t m_mask;
        std::vector<Cell> m_cells;
        alignas(64) std::atomic<std::size_t> m_head;
        alignas(64) std::atomic<std::size_t> m_tail;
    };

    // ----------------- Job descriptor pushed to the queue -----------------
    struct Job
    {
        JobGroup* group{ nullptr };
        std::size_t shardIndex{ 0 };
        std::size_t shardCount{ 1 };
    };

    // ----------------- Job System -----------------
    class JobSystem
    {
    public:
        JobSystem(uint32_t threads, std::size_t queueCapacityPow2, FrameArena* arena)
            : m_threads(threads),
            m_queue(queueCapacityPow2),
            m_arena(arena),
            m_taskSem(0) {
            m_shutdown.store(false, std::memory_order_relaxed);
            for (uint32_t i = 0; i < threads; ++i) {
                m_threads[i] = std::thread([this, i] { workerLoop(i); });
            }
        }

        ~JobSystem() {
            m_shutdown.store(true, std::memory_order_release);
            // Wake everything up so threads can exit.
            m_taskSem.release(std::numeric_limits<int>::max() / 2);
            for (auto& t : m_threads) if (t.joinable()) t.join();
        }

        void reset() {
            m_arena->reset();
        }

        // ----------------- API: single job -----------------
        template <class Fn>
        JobHandle job(std::size_t shards, JobAwaitPoint ap, Fn&& fn) {
            // Store callable once in the group; each shard references it.
            const CallablePayload payload = make_callable(std::forward<Fn>(fn), m_arena);

            // Allocate group from control arena
            JobGroup* group = allocGroup(static_cast<uint32_t>(shards), ap, payload);

            // Enqueue one Job per shard
            for (std::size_t s = 0; s < shards; ++s) {
                Job j;
                j.group = group;
                j.shardIndex = s;
                j.shardCount = shards;
                // Busy until pushed; for production you may want backoff or overflow list.
                while (!m_queue.enqueue(j)) { /* optional pause */ }
            }
            m_taskSem.release(static_cast<int>(shards));

            //if (ap == JobAwaitPoint::Immediate) {                                             _____________________________________
            //    wait(group);
            //} else {
            //    // Defer waiting; engine should call waitAwaitPoint(ap) later.
            //    // You can also stash handles per await point if you prefer.
            //}
            return group;
        }

        // ----------------- API: parallel for over [begin, end) -----------------
        // body(index, threadIndex) is called for each i in [begin,end)
        template <class Fn>
        JobHandle for_range(std::size_t begin, std::size_t end, std::size_t shardCount,
                            JobAwaitPoint ap, Fn&& body) {
            const std::size_t N = (end > begin) ? (end - begin) : 0;
            if (N == 0) return nullptr;

            // Wrap 'body' into shard lambda: compute my subrange and iterate.
            auto shardFn = [=](std::size_t shardIdx, std::size_t totalShards, std::size_t /*threadIndex*/) {
                // balanced static partition
                const std::size_t base = N / totalShards;
                const std::size_t rem = N % totalShards;
                const std::size_t myCount = base + (shardIdx < rem ? 1 : 0);
                const std::size_t offset = base * shardIdx + (shardIdx < rem ? shardIdx : rem);
                const std::size_t myStart = begin + offset;
                const std::size_t myStop = myStart + myCount;
                for (std::size_t i = myStart; i < myStop; ++i) {
                    body(i, shardIdx); // pass shardIdx as a stable small "threadish" id if you want
                }
                };

            return job(shardCount, ap, std::move(shardFn));
        }

        struct State
        {
            std::atomic<std::size_t> next; // next start index to claim
            std::size_t end;
            std::size_t chunk;
        };

        // Wrap body + state so the callable destructor can free State from the arena.
        template <class Body>
        struct ChunkedRangeFunctor
        {
            State* st;
            FrameArena* arena;
            Body body;

            ~ChunkedRangeFunctor() {
                st->~State();
                //arena->deallocate(st, sizeof(State), alignof(State));
                arena->deallocate(st, sizeof(State));
            }
            void operator()(std::size_t /*shardIdx*/, std::size_t /*totalShards*/, std::size_t /*threadIndex*/) {
                for (;;) {
                    // relaxed is fine: each worker owns its claimed range exclusively
                    std::size_t start = st->next.fetch_add(st->chunk, std::memory_order_relaxed);
                    if (start >= st->end) break;
                    std::size_t stop = std::min(start + st->chunk, st->end);
                    // Tight inner loop over a contiguous block -> good locality
                    for (std::size_t i = start; i < stop; ++i) {
                        body(i);
                    }
                }
            }
        };



        template <class Fn>
        JobHandle for_range_chunked(std::size_t begin,
                                    std::size_t end,
                                    std::size_t chunk,
                                    std::size_t shardCount,      // number of worker shards to run (e.g., = threads)
                                    JobAwaitPoint ap,
                                    Fn&& body)
        {
            //using F = std::decay_t<Fn>;
            //auto functor = ChunkedRangeFunctor<F>{ st, m_arena, std::forward<Fn>(body) };

            //// Submit shardCount identical shards that all draw work from the shared State
            //return job(shardCount, ap,
            //           std::move(functor));


            if (end <= begin || shardCount == 0) return nullptr;

            const std::size_t N = end - begin;

            if (chunk == 0) {
                // ---- Static blocked partition across shards ----
                // Remainder distributed to earliest shards so LAST shard gets the least work.
                // base = floor(N / S), rem = N % S
                // shard k gets count = base + (k < rem ? 1 : 0)
                // shard S-1 has count = base (the least, when rem > 0)
                auto staticFn = [=](std::size_t shardIdx, std::size_t totalShards, std::size_t /*tid*/) {
                    const std::size_t S = totalShards;
                    if (shardIdx >= S) return;

                    const std::size_t base = N / S;
                    const std::size_t rem = N % S;

                    const std::size_t myCount = base + (shardIdx < rem ? 1 : 0);
                    if (myCount == 0) return; // more shards than work

                    const std::size_t offset =
                        base * shardIdx + (shardIdx < rem ? shardIdx : rem);

                    const std::size_t myStart = begin + offset;
                    const std::size_t myStop = myStart + myCount;

                    for (std::size_t i = myStart; i < myStop; ++i) {
                        body(i);
                    }
                    };

                // Submit exactly shardCount shards that each walk their own static block.
                return job(shardCount, ap, std::move(staticFn));
            }

            // Allocate State in the *same arena* as the callable so we can free it when the callable dies.
            void* stMem = m_arena->allocate(sizeof(State), alignof(State));
            auto* st = new (stMem) State{ begin, end, std::max<std::size_t>(1, chunk) };

            using F = std::decay_t<Fn>;
            auto functor = ChunkedRangeFunctor<F>{ st, m_arena, std::forward<Fn>(body) };

            // Submit shardCount identical shards that all draw work from the shared State
            return job(shardCount, ap,
                       std::move(functor));
        }


        // Wait for everything submitted to a specific await point to finish.
        // If you keep handles externally, just call MWork::wait(handle) at the right phase instead.
        void waitAwaitPoint(JobAwaitPoint ap) {
            // Drain: repeatedly try to observe zero outstanding groups at this await point.
            // Simpler approach: spin on 'inflight' and sleep efficiently by waiting on a per-AP semaphore.
            // For now we just block on a barrier loop that watches a counter.
            // (Hook up your engine’s phase system and call 'wait(h)' for each handle you queued to that AP.)
            // This method is a placeholder—opt for explicit handle tracking for determinism.
        }

    private:
        // Worker loop
        void workerLoop(std::size_t threadIndex) {
            // Optional: set affinity here for cache locality.
            for (;;) {
                m_taskSem.acquire();
                if (m_shutdown.load(std::memory_order_acquire)) return;

                Job job;
                // If there’s a race between acquire and empty queue due to other workers stealing work,
                // just keep trying until we get one (or observe shutdown).
                while (!m_queue.dequeue(job)) {
                    if (m_shutdown.load(std::memory_order_acquire)) return;
                    // spin a little—queue should be non-empty because a token was released
                }

                JobGroup* g = job.group;
                // Invoke shared callable
                g->payload.invoke(g->payload.obj, job.shardIndex, job.shardCount, threadIndex);

                // Signal completion
                const uint32_t prev = g->pending.fetch_sub(1, std::memory_order_acq_rel);
                if (prev == 1) {
                    // We are last: destroy callable, release waiter, and free group
                    if (g->payload.destroy)
                        g->payload.destroy(g->payload.obj, g->payload.arena);       
                    g->done.release();
                    freeGroup(g);
                    
                }
            }
        }

        JobGroup* allocGroup(uint32_t shards, JobAwaitPoint ap, const CallablePayload& payload) {
            void* mem = m_arena->allocate(sizeof(JobGroup), alignof(JobGroup));
            return new (mem) JobGroup(shards, ap, payload);
        }

        void freeGroup(JobGroup* g) {
            g->~JobGroup();
            m_arena->deallocate(g, sizeof(JobGroup));
        }

    private:
        std::vector<std::thread> m_threads;
        MpmcQueue<Job>           m_queue;
        std::atomic<bool>        m_shutdown{ false };
        std::counting_semaphore<std::numeric_limits<int>::max()> m_taskSem;
        FrameArena*               m_arena = nullptr;

    };

    // ----------------- Simple façade (matches your examples) -----------------
    namespace detail
    {
        inline JobSystem*& instance() {
            static JobSystem* g = nullptr;
            return g;
        }
    }

    inline void init(uint32_t threads, std::size_t queueCapacityPow2, FrameArena* arena) {
        detail::instance() = new JobSystem{ threads, queueCapacityPow2, arena };
    }

    inline void shutdown() {
        delete detail::instance();
        detail::instance() = nullptr;
    }

    inline void reset() {
        detail::instance()->reset();
    }

    inline void waitAwaitPoint(JobAwaitPoint ap) {
        detail::instance()->waitAwaitPoint(ap);
    }

    // In practice a "parallel for" using 'shards' as the split count.
    // The lambda signature should be: void(shardIndex, shardCount, threadIndex)
    template <class Fn>
    inline JobHandle job(std::size_t shards, JobAwaitPoint ap, Fn&& fn) {
        return detail::instance()->job(shards, ap, std::forward<Fn>(fn));
    }

    // Range-based parallel for convenience. body(i, shardIndex).
    template <class Fn>
    inline JobHandle for_range(std::size_t begin, std::size_t end, std::size_t shards,
                               JobAwaitPoint ap, Fn&& body) {
        return detail::instance()->for_range(begin, end, shards, ap, std::forward<Fn>(body));
    }

    template <class Fn>
    inline JobHandle for_range_chunked(std::size_t begin, std::size_t end, std::size_t chunk, std::size_t shards,
                               JobAwaitPoint ap, Fn&& body) {
        if (ap == JobAwaitPoint::Immediate) {
            JobHandle result = detail::instance()->for_range_chunked(begin, end, chunk, shards, ap, std::forward<Fn>(body));
            wait(result);
            return result;
        } else
            return detail::instance()->for_range_chunked(begin, end, chunk, shards, ap, std::forward<Fn>(body));
    }

    template <class Fn>
    inline JobHandle for_loop(std::size_t begin, std::size_t end, std::size_t shards, Fn&& body) {
        return MWork::for_range_chunked(begin, end, 0, shards, MWork::JobAwaitPoint::Immediate, body);
    }

} // namespace MWork

#endif