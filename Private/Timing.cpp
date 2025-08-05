#include "Timing.h"
#include "Engine.h"

double Timing::deltaTime() {
	return Engine::getInstance()->deltaTime();
}
float Timing::deltaTimeF() {
	return static_cast<float>(Engine::getInstance()->deltaTime());
}

double Timing::fixedDeltaTime() {
	return Engine::getInstance()->fixedDeltaTime();
}

float Timing::fixedDeltaTimeF() {
	return static_cast<float>(Engine::getInstance()->fixedDeltaTime());
}

double Timing::totalTime() {
	return Engine::getInstance()->totalTime();
}

float Timing::totalTimeF() {
	return static_cast<float>(Engine::getInstance()->totalTime());
}


size_t Timing::totalFrames() {
	return Engine::getInstance()->totalFrames();
}