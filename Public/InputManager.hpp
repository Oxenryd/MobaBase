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

	void pollEvents() {
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
};

#endif // INPUTMANAGER_HPP