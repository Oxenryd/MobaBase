#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP



#include "WindowSurface.h"
#include "Delegate.hpp"
//#include "GlobalMacros.h"



//Event<uint8_t, KeysBitfield> onKeyDown;
//Event<uint8_t, KeysBitfield> onKeyUp;

class InputManager
{
private:
	WindowSurface* m_ws;
	std::set<uint16_t> m_keyButtonDownMap;

public:
    ~InputManager() {}
	InputManager(WindowSurface* surface) :
		m_ws{surface} {
#ifdef BUILD_WIN
        m_ws->onRawInput.subscribe(&HandleRawInputWin32);
		m_ws->onKeyEvent.subscribe(&handleKeyEvent);
#endif
	}

	INLINE void update() {
#ifdef BUILD_WIN
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif
	}

	void handleKeyEvent(KeyEvent event) {

		onKeyEvent.notify(event); // for manual handling

		auto it = m_keyButtonDownMap.find(static_cast<uint16_t>(event.code));
		if (event.action == KeyAction::Release) {
			if (it != m_keyButtonDownMap.end()) {
				m_keyButtonDownMap.erase(it);
				onKeyUp.notify(event);
			}
		} else {
			if (it == m_keyButtonDownMap.end()) {
				m_keyButtonDownMap.insert(static_cast<uint16_t>(event.code));
				onKeyDown.notify(event);
			} else {
				event.action = KeyAction::Hold;
				onKeyHeld.notify(event);
			}
		}
		
	}

	// EVENTS
	Event<KeyEvent> onKeyEvent;
	Event<KeyEvent> onKeyDown;
	Event<KeyEvent> onKeyHeld;
	Event<KeyEvent> onKeyUp;
};



#endif // INPUTMANAGER_HPP