#ifndef TIMERSYSTEM_H
#define TIMERSYSTEM_H

#include <immintrin.h>
#include <thread>
#include <vector>
#include <atomic>
#include <barrier>
#include <functional>
#include <cassert>

#include "Timer.h"
#include "Bits.hpp"
#include "Delegate.hpp"

class TimerSystem
{
private:
    friend class Engine;
    inline static TimerSystem* s_currentProvider;
    size_t m_num_threads;
    std::vector<std::thread> m_workers;
    std::barrier<> m_barrier;
    std::atomic<bool> m_stop = false;
    double m_deltaTime;
    std::vector<double> m_incCounters;
    std::vector<double> m_incTimes;
    std::vector<double> m_decCounters;
    std::vector<double> m_decResets;
    std::vector<SizedBitField<uint8_t>> m_incStates;
    std::vector<SizedBitField<uint8_t>> m_decStates;
    std::vector<std::vector<uint32_t>> m_timedIncIndices;
    std::vector<std::vector<uint32_t>> m_timedDecIndices;
    std::vector<Event<>> m_incOnTimes;
    std::vector<Event<>> m_decOnTimes;

    void _workerLoop(size_t threadIndex);

    void _processTimers(
        std::vector<double>& timers, std::vector<double>& targets,
        double deltaTime, bool is_inc, size_t threadIndex);

public:
    TimerSystem(size_t num_threads);

    ~TimerSystem();

    void update(double deltaTime);

    void startTimer(Timer timer);
    void stopTimer(const Timer& timer);
    void resetTimer(const Timer& timer);

    Timer createTimer(bool increasing, double time, double start = 0.0);
};

#endif