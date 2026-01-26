#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <thread>
#include <chrono>

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