#include "TimerSystem.h"

void TimerSystem::_workerLoop(const size_t threadIndex) {
    while (!m_stop) {
        m_barrier.arrive_and_wait();

        if (m_stop) break;
        m_timedIncIndices[threadIndex].clear();
        m_timedDecIndices[threadIndex].clear();
        _processTimers(m_incCounters, m_incTimes, m_deltaTime, true, threadIndex);
        _processTimers(m_decCounters, m_decResets, m_deltaTime, false, threadIndex);

        m_barrier.arrive_and_wait();
    }
}

//void TimerSystem::_processTimers(std::vector<double>& timers, std::vector<double>& targets, double deltaTime, bool is_inc, size_t threadIndex) {
//    constexpr size_t avx_width = 4;
//    __m256d delta = _mm256_set1_pd(is_inc ? deltaTime : -deltaTime);
//
//    size_t chunk_size = (timers.size() + m_num_threads - 1) / m_num_threads;
//    size_t start = threadIndex * chunk_size;
//    size_t end = std::min(start + chunk_size, timers.size());
//
//    size_t i = start;
//    for (; i + avx_width <= end; i += avx_width) {
//        __m256d current = _mm256_load_pd(&timers[i]);
//        __m256d updated = _mm256_add_pd(current, delta);
//        _mm256_store_pd(&timers[i], updated);
//    }
//    for (; i < end; ++i) {
//        timers[i] += (is_inc ? deltaTime : -deltaTime);
//    }
//}

void TimerSystem::_processTimers(std::vector<double>& timers,
                                 const std::vector<double>& targets,
                                 const double deltaTime,
                                 const bool is_inc,
                                 const size_t threadIndex) {
    constexpr size_t avx_width = 4;

    const size_t total = timers.size();
    const size_t chunk_size = (total + m_num_threads - 1) / m_num_threads;
    const size_t start = threadIndex * chunk_size;
    const size_t end = std::min(start + chunk_size, total);

    const __m256d delta = _mm256_set1_pd(is_inc ? deltaTime : -deltaTime);

    size_t i = start;
    for (; i + avx_width <= end; i += avx_width) {
        // Load counters and targets
        const __m256d counter = _mm256_loadu_pd(timers.data() + i);
        __m256d target = _mm256_loadu_pd(targets.data() + i);

        const auto& states = is_inc ? m_incStates : m_decStates;

        // Load states into scalar
        const double mask_vals[4] = {
            static_cast<double>(states[i + 0].hasFlag(TimerState::Running)),
            static_cast<double>(states[i + 1].hasFlag(TimerState::Running)),
            static_cast<double>(states[i + 2].hasFlag(TimerState::Running)),
            static_cast<double>(states[i + 3].hasFlag(TimerState::Running)),
        };

        const __m256d stateMask = _mm256_loadu_pd(mask_vals);
        const __m256d maskedDelta = _mm256_mul_pd(delta, stateMask);

        // Update counters
        __m256d updated = _mm256_add_pd(counter, maskedDelta);
        _mm256_storeu_pd(timers.data() + i, updated);

        // Compare
        __m256d cmp_result;
        if (is_inc)
            cmp_result = _mm256_cmp_pd(updated, target, _CMP_GE_OQ);
        else
            cmp_result = _mm256_cmp_pd(updated, _mm256_setzero_pd(), _CMP_LE_OQ);

        cmp_result = _mm256_and_pd(cmp_result, stateMask);

        // Extract bitmask


        // Process mask
        if (const int mask = _mm256_movemask_pd(cmp_result); mask) {
            for (size_t lane = 0; lane < avx_width; ++lane) {
                if (mask & 1 << lane) {
                    if (is_inc)
                        m_timedIncIndices[threadIndex].push_back(static_cast<uint32_t>(i + lane));
                    else
                        m_timedDecIndices[threadIndex].push_back(static_cast<uint32_t>(i + lane));
                }
            }
        }
    }

    for (; i < end; ++i) {

        if (is_inc) {
            if (!m_incStates[i].hasFlag(TimerState::Running)) continue;
        } else
            if (!m_decStates[i].hasFlag(TimerState::Running)) continue;

        timers[i] += is_inc ? deltaTime : -deltaTime;

        if (is_inc && timers[i] >= targets[i])
            m_timedIncIndices[threadIndex].push_back(static_cast<uint32_t>(i));
        else if (!is_inc && timers[i] <= 0.0)
            m_timedDecIndices[threadIndex].push_back(static_cast<uint32_t>(i));
    }
}


TimerSystem::TimerSystem(const size_t num_threads) :
    m_num_threads(num_threads),
    m_barrier(num_threads + 1),
    m_deltaTime{0.0}
{
    assert(m_num_threads > 0);
    m_workers.reserve(m_num_threads);
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_workers.emplace_back([this, i] { _workerLoop(i); });
        m_timedIncIndices.push_back(std::vector<uint32_t>());
        m_timedDecIndices.push_back(std::vector<uint32_t>());
    }
}

TimerSystem::~TimerSystem() {
    m_stop = true;
    m_barrier.arrive_and_wait();
    for (auto& w : m_workers) {
        if (w.joinable()) w.join();
    }
}

void TimerSystem::update(const double deltaTime) {
    m_deltaTime = deltaTime;

    m_barrier.arrive_and_wait();
    m_barrier.arrive_and_wait();
    
    // Do onTimes
    for (size_t i = 0; i < m_num_threads; ++i) {
        for (const auto& index : m_timedIncIndices[i]) {
            m_incOnTimes[index].notify();
            if (m_incStates[index].hasFlag(TimerState::AutoReset)) {
                m_incCounters[index] = 0.0;
            } else {
                m_incStates[index].clearByEnum(TimerState::Running);
                m_incStates[index].setByEnum(TimerState::Done);
                m_incCounters[index] = m_incTimes[index];
            }
        }
        for (const auto& index : m_timedDecIndices[i]) {
            m_decOnTimes[index].notify();
            if (m_decStates[index].hasFlag(TimerState::AutoReset)) {
                m_decCounters[index] = m_decResets[index];
            } else {
                m_decStates[index].clearByEnum(TimerState::Running);
                m_decStates[index].setByEnum(TimerState::Done);
                m_decCounters[index] = 0.0;
            }
        }
    }
}

void TimerSystem::startTimer(const Timer& timer) {
    switch (timer.m_direction) {
        case TimerCountDirection::Incrementing:
        {
            m_incStates[static_cast<std::size_t>(timer)].clearByEnum(TimerState::Stopped);
            m_incStates[static_cast<std::size_t>(timer)].clearByEnum(TimerState::Done);
            m_incStates[static_cast<std::size_t>(timer)].setByEnum(TimerState::Running);
            m_incCounters[static_cast<std::size_t>(timer)] = 0.0;
        } break;

        case TimerCountDirection::Decrementing:
        {
            m_decStates[static_cast<std::size_t>(timer)].clearByEnum(TimerState::Stopped);
            m_decStates[static_cast<std::size_t>(timer)].clearByEnum(TimerState::Done);
            m_decStates[static_cast<std::size_t>(timer)].setByEnum(TimerState::Running);
            m_decCounters[static_cast<std::size_t>(timer)] = m_decResets[static_cast<std::size_t>(timer)];
        } break;
    }
}

Timer TimerSystem::createTimer(const bool increasing, double time, double start) {

    if (increasing) {
        const auto index = m_incCounters.size();
        m_incCounters.push_back({start});
        m_incTimes.push_back({time});
        m_incStates.push_back(SizedBitField<uint8_t>());
        m_incStates.back().setByEnum(TimerState::AutoReset);
        m_incStates.back().setByEnum(TimerState::Incrementing);
        m_incStates.back().setByEnum(TimerState::Stopped);
        m_incOnTimes.push_back(Event());
        return Timer{ static_cast<uint32_t>(index), TimerCountDirection::Incrementing };
    }
    { // else
        auto actualStart = start == 0.0 ? time : start;
        const auto index = m_decCounters.size();
        m_decCounters.push_back({ actualStart });
        m_decResets.push_back({ time });
        m_decStates.push_back(SizedBitField<uint8_t>());
        m_decStates.back().setByEnum(TimerState::AutoReset);
        m_decStates.back().setByEnum(TimerState::Decrementing);
        m_decStates.back().setByEnum(TimerState::Stopped);
        m_decOnTimes.push_back(Event());
        return Timer{ static_cast<uint32_t>(index), TimerCountDirection::Decrementing };
    }
}