#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

class Engine;
class Arena;
class HeapArena;
class FrameArena;

enum class TimerState : uint32_t
{
	Invalid			= 0,
	Decrementing	= 1 << 0,
	Incrementing	= 1 << 1,
	AutoReset		= 1 << 2,
	Stopped			= 1 << 3,
	Running			= 1 << 4,
	Done			= 1 << 5
};

enum class TimerCountDirection : uint8_t
{
	Incrementing,
	Decrementing
};

struct Timer
{
private:
	friend class TimerSystem;
	friend class Engine;
	friend class HeapArena;
	friend class FrameArena;

	uint32_t m_timerIndex;
	TimerCountDirection m_direction;
	Timer(uint32_t index, TimerCountDirection direction);
	Timer(const Timer& other);

public:
	Timer() = default;
	~Timer() = default;
	explicit operator uint32_t() const {
		return m_timerIndex;
	}
	explicit operator size_t() const {
		return m_timerIndex;
	}

	//float ratio();
	//void start();
	//void addState(TimerState state);
	//void removeState(TimerState state);
	//Event<void>& getOnTimeEvent();
};

#endif