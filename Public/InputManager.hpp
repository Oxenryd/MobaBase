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
	MouseState m_lastMouseState;
	MouseState m_currentMouseState;
	glm::i16vec2 m_lastDeltaMax{};

	bool m_gotMousePosThisFrame = false;
public:
    ~InputManager() {}
	InputManager(WindowSurface* surface) :
		m_ws{surface} {
#ifdef BUILD_WIN
        m_ws->onRawInput.subscribe(&HandleRawInputWin32);
		m_ws->onKeyEvent.subscribe( [this](KeyEvent event) -> void
								   {
									   handleKeyEvent(this, event);
								   });

		m_ws->onMouse.subscribe( [this](MouseState mState) -> void
			{
				m_currentMouseState = mState;
				auto delta = m_currentMouseState.relativePosition - m_lastMouseState.relativePosition;
				if (delta.x * delta.x + delta.y * delta.y > m_lastDeltaMax.x * m_lastDeltaMax.x + m_lastDeltaMax.y * m_lastDeltaMax.y) {
					m_lastDeltaMax = delta;
				}	
				m_currentMouseState.lastPositionDelta = m_lastDeltaMax;
			});

#endif
	}

	INLINE void update() {

		
		m_lastDeltaMax = { 0,0 };
		m_currentMouseState.lastPositionDelta = m_lastDeltaMax;

		for (auto& code : m_keyButtonDownMap) {
			onKeyHold.notify(static_cast<KeyCode>(code));
		}

#ifdef BUILD_WIN
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif

		m_lastMouseState = m_currentMouseState;
	}

	MouseState& currentMouseState() const { return const_cast<MouseState&>(m_currentMouseState); }
	MouseState& lastMouseState() const { return const_cast<MouseState&>(m_lastMouseState); }

	static void handleKeyEvent(InputManager* _this, KeyEvent event) {

		_this->onKeyEvent.notify(event); // for manual handling

		auto it = _this->m_keyButtonDownMap.find(static_cast<uint16_t>(event.code));
		if (event.action == KeyAction::Release) {
			if (it != _this->m_keyButtonDownMap.end()) {
				_this->m_keyButtonDownMap.erase(it);
				_this->onKeyUp.notify(event.code);
			}
		} else {
			if (it == _this->m_keyButtonDownMap.end()) {
				_this->m_keyButtonDownMap.insert(static_cast<uint16_t>(event.code));
				_this->onKeyDown.notify(event.code);
			}
		}
		
	}

	// EVENTS
	Event<KeyEvent> onKeyEvent;
	Event<KeyCode> onKeyDown;
	Event<KeyCode> onKeyHold;
	Event<KeyCode> onKeyUp;
};



#endif // INPUTMANAGER_HPP