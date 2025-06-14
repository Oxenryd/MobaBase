#include "Timer.h"
#include "Arena.hpp"
#include "TimerSystem.h"

Timer::Timer(const uint32_t index, const TimerCountDirection direction) : 
	m_timerIndex{ index }, m_direction{ direction }
{}

Timer::Timer(const Timer& other) : 
	m_timerIndex{other.m_timerIndex},
	m_direction{other.m_direction}
{}