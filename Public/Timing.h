#ifndef TIMING_H
#define TIMING_H


class Timing
{
public:
	static double deltaTime();
	static float deltaTimeF();
	static double fixedDeltaTime();
	static float fixedDeltaTimeF();
	static double totalTime();
	static float totalTimeF();
	static size_t totalFrames();
};

#endif