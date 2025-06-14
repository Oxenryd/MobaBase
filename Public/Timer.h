#include <cstdint>

#ifndef TIMER_H
#define TIMER_H

class Engine;
class Arena;
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
	uint32_t m_timerIndex;
	TimerCountDirection m_direction;
	Timer(const uint32_t index, const TimerCountDirection direction);
	Timer(const Timer& other);

public:

	inline operator uint32_t() {
		return static_cast<uint32_t>(m_timerIndex);
	}
	inline operator size_t() {
		return static_cast<size_t>(m_timerIndex);
	}

	//float ratio();
	//void start();
	//void addState(TimerState state);
	//void removeState(TimerState state);
	//Event<void>& getOnTimeEvent();
};

#endif