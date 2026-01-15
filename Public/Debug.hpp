#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <thread>
#include <chrono>

inline void assume(bool cond) {
#if defined(__clang__) || defined(__GNUC__)
	if (!cond) std::__terminate();
#elif defined(_MSC_VER)
	__assume(cond);
#else
	(void)cond;
#endif
}

namespace Console
{
	inline void printLine(const std::string& msg) {
		std::cout << msg << '\n';
	}
}

namespace Debug
{
	inline void sleepBlock(const uint32_t milliseconds) {
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}
}

#endif