#ifndef PROFILER_HPP
#define PROFILER_HPP

#include <atomic>
#include <cstdint>
#include <string_view>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <chrono>

#include "GlobalMacros.h"

namespace Prof
{

    using Clock = std::chrono::steady_clock;

    struct Event
    {
        const char* name;           // static string only to avoid allocs
        uint64_t    ts_begin_us;    // microseconds since start
        uint64_t    ts_end_us;      // microseconds since start
        uint32_t    tid;            // numeric thread id
        uint32_t    depth;          // nesting depth
    };

    struct ThreadBuffer
    {
        std::vector<Event> events;
        std::vector<uint32_t> depth_stack; // track nesting
        uint32_t tid = 0;
    };

    inline uint64_t rel_us(Clock::time_point t0, Clock::time_point t) {
        return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(t - t0).count();
    }

    class Profiler
    {
    public:
        static Profiler& instance() { static Profiler P; return P; }

        void startSession(const char* name, const char* outPath = "trace.json") {
            session_name_ = name;
            out_path_ = outPath;
            start_time_ = Clock::now();
            enabled_.store(true, std::memory_order_relaxed);
        }
        void endSession() {
            enabled_.store(false, std::memory_order_relaxed);
            flush_to_file();
            // free buffers if you want
        }

        bool enabled() const { return enabled_.load(std::memory_order_relaxed); }

        ThreadBuffer& tls() {
            thread_local ThreadBuffer tb;
            if (tb.tid == 0) tb.tid = get_tid();
            return tb;
        }

        void pushScope(const char* name) {
            if (!enabled()) return;
            auto& tb = tls();
            Event ev{};
            ev.name = name;
            ev.ts_begin_us = rel_us(start_time_, Clock::now());
            ev.tid = tb.tid;
            ev.depth = (uint32_t)tb.depth_stack.size();
            tb.events.emplace_back(ev);
            tb.depth_stack.push_back((uint32_t)tb.events.size() - 1);
        }
        void popScope() {
            if (!enabled()) return;
            auto& tb = tls();
            const auto idx = tb.depth_stack.back();
            tb.depth_stack.pop_back();
            tb.events[idx].ts_end_us = rel_us(start_time_, Clock::now());
        }

        // Call once per frame from main thread to collect & optionally clear
        void frameMark(uint64_t frame_index) {
            if (!enabled()) return;
            (void)frame_index;
            std::scoped_lock lk(all_mutex_);
            // snapshot thread-local into global list (we don’t clear; we flush on endSession)
            // optional: move & clear per-frame to keep files small
            // no-op here; we’ll walk TLS at flush time via a registry if desired
        }

    private:
        Profiler() = default;

        static uint32_t get_tid() {
            // Portable-ish hash of std::thread::id
            auto h = std::hash<std::thread::id>{}(std::this_thread::get_id());
            return (uint32_t)(h & 0xFFFFFFFFu);
        }

        void flush_to_file() {
            std::scoped_lock lk(all_mutex_);
            // We cannot enumerate TLS of other threads portably;
            // pattern: register TLS buffers at first use:
        }

    public:
        // Registration so we can flush all threads:
        void registerBuffer(ThreadBuffer* tb) {
            std::scoped_lock lk(all_mutex_);
            all_threads_.push_back(tb);
        }

        void writeTrace() {
            std::ofstream f(out_path_, std::ios::binary);
            f << "{\"traceEvents\":[\n";
            bool first = true;
            for (auto tb : all_threads_) {
                for (auto& e : tb->events) {
                    if (e.ts_end_us == 0)
                        continue; // still open
                    if (!first) f << ",\n";
                    first = false;
                    // Chrome trace "B"/"E" pairs are verbose; we can write complete events ("X"):
                    f << "{"
                        << "\"name\":\"" << e.name << "\","
                        << "\"ph\":\"X\","
                        << "\"ts\":" << e.ts_begin_us << ","
                        << "\"dur\":" << (e.ts_end_us - e.ts_begin_us) << ","
                        << "\"pid\":1,"
                        << "\"tid\":" << e.tid
                        << "}";
                }
            }
            f << "\n]}";
        }

        void flush_to_file_public() { 
            writeTrace(); 
        }

    private:
        std::atomic<bool> enabled_{ false };
        Clock::time_point start_time_{};
        std::string session_name_;
        std::string out_path_;
        std::mutex all_mutex_;
        std::vector<ThreadBuffer*> all_threads_;
    };

    // RAII scope
    class Scope
    {
    public:
        Scope(const char* name) : enabled_(Profiler::instance().enabled()) {
            if (enabled_) {
                auto& P = Profiler::instance();
                auto& tb = P.tls();
                // one-time register
                static thread_local bool registered = false;
                if (!registered) { P.registerBuffer(&tb); registered = true; }
                P.pushScope(name);
            }
        }
        ~Scope() { if (enabled_) Profiler::instance().popScope(); }
    private:
        bool enabled_;
    };

} // namespace Prof

// Macros
//#if !defined(PROFILER)
//#define PROFILER 1 // flip to 0 for compile-out
//#endif

#if PROFILER
#define PROFILE_BEGIN_SESSION(name, path) ::Prof::Profiler::instance().startSession(name, path)
#define PROFILE_END_SESSION()             do{ auto& P=::Prof::Profiler::instance(); P.endSession(); P.flush_to_file_public(); }while(0)
#define PROFILE_FRAME_MARK(i)             ::Prof::Profiler::instance().frameMark(i)
#define PROFILE_SCOPE(name_literal)       ::Prof::Scope CONCAT(_prof_scope_, __LINE__){name_literal}
#define PROFILE_FUNC()                    PROFILE_SCOPE(__func__)
#else
#define PROFILE_BEGIN_SESSION(name, path)  ((void)0)
#define PROFILE_END_SESSION()              ((void)0)
#define PROFILE_FRAME_MARK(i)              ((void)0)
#define PROFILE_SCOPE(name_literal)        ((void)0)
#define PROFILE_FUNC()                     ((void)0)
#endif

#endif