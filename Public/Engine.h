#ifndef ENGINE_H
#define ENGINE_H

#include <cstdint>
#include "InputManager.hpp"
#include "WindowSurface.h"
#include "Debug.hpp"

class Engine
{
private:

public:
	Engine() {}
	
	inline void update(WindowSurface* wndPtr, InputManager* inputPtr);
};

#endif //ENGINE_H