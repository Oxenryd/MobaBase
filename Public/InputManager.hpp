#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include "WindowSurface.h"

class InputManager
{


private:
	WindowSurface* m_ws;
public:
	InputManager(WindowSurface* surface) :
		m_ws{surface} {}


};

#endif // INPUTMANAGER_HPP