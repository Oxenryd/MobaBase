#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <glm/glm.hpp>
#include "WindowContext.h"
#include "Delegate.hpp"
#include "InputTypes.h"

//#include "GlobalMacros.h"

//Event<uint8_t, KeysBitfield> onKeyDown;
//Event<uint8_t, KeysBitfield> onKeyUp;
//enum class MouseButton : uint8_t
//{
//	None = 0x00,
//	Left = 0x01,
//	Right = 0x02,
//	ShiftKey = 0x04,
//	ControlKey = 0x08,
//	Middle = 0x10,
//	M4 = 0x20,
//	M5 = 0x40,
//	_ENUM_END = 0x80
//};

class InputManager
{
private:
	WindowContext* m_ws;
	std::set<uint16_t> m_keyButtonDownMap;
	uint8_t m_mButtonsDown = 0;
	bool m_ignoreNextMouseDelta = false;
	MouseState m_lastMouseState;
	MouseState m_currentMouseState;
	glm::i16vec2 m_lastDeltaMax{};

	bool m_gotMousePosThisFrame = false;
	float m_deltaTime{};



	void _notifyMouseButtonEvent(uint8_t bit, KeyAction action) {

		switch (bit) {

			default: return;

			case 0x01: switch (action) {
				default: return;
				case KeyAction::Press: onMouseLeftDown.notify(m_currentMouseState); return;
				case KeyAction::Hold: onMouseLeftHold.notify(m_lastMouseState); return;
				case KeyAction::Release: onMouseLeftUp.notify(m_currentMouseState); return;
			}

			case 0x02: switch (action) {
				default: return;
				case KeyAction::Press: onMouseRightDown.notify(m_currentMouseState); return;
				case KeyAction::Hold: onMouseRightHold.notify(m_lastMouseState); return;
				case KeyAction::Release: onMouseRightUp.notify(m_currentMouseState); return;
			}

			case 0x10: switch (action) {
				default: return;
				case KeyAction::Press: onMouseMiddleDown.notify(m_currentMouseState); return;
				case KeyAction::Hold: onMouseMiddleHold.notify(m_lastMouseState); return;
				case KeyAction::Release: onMouseMiddleUp.notify(m_currentMouseState); return;
			}

			case 0x20: switch (action) {
				default: return;
				case KeyAction::Press: onMouseM4Down.notify(m_currentMouseState); return;
				case KeyAction::Hold: onMouseM4Hold.notify(m_lastMouseState); return;
				case KeyAction::Release: onMouseM4Up.notify(m_currentMouseState); return;
			}

			case 0x40: switch (action) {
				default: return;
				case KeyAction::Press: onMouseM5Down.notify(m_currentMouseState); return;
				case KeyAction::Hold: onMouseM5Hold.notify(m_lastMouseState); return;
				case KeyAction::Release: onMouseM5Up.notify(m_currentMouseState); return;
			}

		}

	}

public:
    ~InputManager() {}
	InputManager(WindowContext* surface) :
		m_ws{surface} {
#ifdef BUILD_WIN
		m_ws->onMouseRaw.subscribe( [this](MouseDataRaw raw) -> void
								   {
									   m_currentMouseState.lastPositionDelta += glm::ivec2{ raw.deltaX, raw.deltaY };// *std::max(static_cast<int32_t>(1.0f / m_deltaTime * m_mouseSense), 1);
									   m_currentMouseState.absoluteScreenPosition = glm::ivec2{ raw.absX, raw.absY };
								   });
		m_ws->onKeyEvent.subscribe( [this](KeyEvent event) -> void
								   {
									   handleKeyEvent(this, event);
								   });

		m_ws->onMouseButton.subscribe( [this](MouseState mState) -> void
			{
									m_currentMouseState.buttonState = mState.buttonState;
			});
		m_ws->onMouseWheel.subscribe([this](MouseState mState) -> void
			 {
									m_currentMouseState.wheel = mState.wheel;
			});

#endif
	}

	INLINE void update(double dt) {

		m_deltaTime = static_cast<float>(dt);
		//m_lastDeltaMax = { 0,0 };
		//m_currentMouseState.lastPositionDelta = m_lastDeltaMax;
		m_currentMouseState.lastPositionDelta = glm::ivec2{ 0, 0 };


		for (auto& code : m_keyButtonDownMap) {
			onKeyHold.notify(static_cast<KeyCode>(code));
		}

		for (uint8_t i = 0; i < 8; ++i) {
			uint8_t mask = 1 << i;
			if (m_mButtonsDown & mask)
				_notifyMouseButtonEvent(mask, KeyAction::Hold);
		}

#ifdef BUILD_WIN
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif

		// MouseButtons
		//uint8_t last = m_lastMouseState.buttonState.getField();
		uint8_t curr = m_currentMouseState.buttonState.getField();
		for (uint8_t i = 0; i < 8; ++i) {
			uint8_t mask = 1 << i;
			if (m_mButtonsDown & mask) {
				if (!(curr & mask))
					_notifyMouseButtonEvent(mask, KeyAction::Release);
			} else if (curr & mask)
				_notifyMouseButtonEvent(mask, KeyAction::Press);
		}
		m_mButtonsDown = curr;
		m_lastMouseState = m_currentMouseState;
	}

	MouseState& currentMouseState() const { return const_cast<MouseState&>(m_currentMouseState); }
	MouseState& lastMouseState() const { return const_cast<MouseState&>(m_lastMouseState); }

#ifdef BUILD_WIN
	void mouseToWindowCenter() {
		m_ignoreNextMouseDelta = true;
		POINT pt{ m_ws->width / 2, m_ws->height / 2 };
		ClientToScreen(m_ws->windowHandle, &pt);
		SetCursorPos(pt.x, pt.y);
	}

	void enableRelativeMouse() {
		// Hide cursor
		while (ShowCursor(FALSE) >= 0); // ensure hidden

		// Confine to client area
		RECT rect;
		GetClientRect(m_ws->windowHandle, &rect);
		POINT tl = { rect.left, rect.top };
		POINT br = { rect.right, rect.bottom };
		ClientToScreen(m_ws->windowHandle, &tl);
		ClientToScreen(m_ws->windowHandle, &br);
		RECT clipRect = { tl.x, tl.y, br.x, br.y };
		ClipCursor(&clipRect);
	}

	void disableRelativeMouse() {
		// Show cursor
		while (ShowCursor(TRUE) < 0); // ensure visible

		// Release cursor
		ClipCursor(nullptr);
	}

#endif

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
	
	// Keyboard
	Event<KeyEvent> onKeyEvent;
	Event<KeyCode> onKeyDown;
	Event<KeyCode> onKeyHold;
	Event<KeyCode> onKeyUp;


	// Mouse
	Event<MouseState> onMouseButtonDown;
	Event<MouseState> onMouseButtonHold;
	Event<MouseState> onMouseButtonUp;

	Event<MouseState> onMouseLeftDown;
	Event<MouseState> onMouseLeftHold;
	Event<MouseState> onMouseLeftUp;
	Event<MouseState> onMouseRightDown;
	Event<MouseState> onMouseRightHold;
	Event<MouseState> onMouseRightUp;
	Event<MouseState> onMouseMiddleDown;
	Event<MouseState> onMouseMiddleHold;
	Event<MouseState> onMouseMiddleUp;
	Event<MouseState> onMouseM4Down;
	Event<MouseState> onMouseM4Hold;
	Event<MouseState> onMouseM4Up;
	Event<MouseState> onMouseM5Down;
	Event<MouseState> onMouseM5Hold;
	Event<MouseState> onMouseM5Up;
};


class Input
{
private:
	static constexpr uint16_t STANDARD_WIDTH = 1280;
	static constexpr uint16_t STANDARD_HEIGHT = 800;
public:

	template <std::floating_point T, std::floating_point U>
	INLINE static glm::vec2 scaledMouseMovementVec2(
		const MouseState& mState,
		T dt, U sensitivity,
		glm::highp_u16vec2 resolution = { STANDARD_WIDTH , STANDARD_HEIGHT }) {

		//auto rec = 0.00025f / dt;
		glm::vec2 scaledRes = glm::vec2{ static_cast<float>(resolution.x), static_cast<float>(resolution.y) } * INPUT_RESO_SCALE_FACTOR;
		float sense = static_cast<float>(sensitivity * sensitivity);
		return glm::vec2{
			-(float)mState.lastPositionDelta.x * sense * scaledRes.x,
			-(float)mState.lastPositionDelta.y * sense * scaledRes.y
		};
	}
	template <std::floating_point T, std::floating_point U>
	INLINE static glm::vec3 scaledMouseMovementVec3(
		const MouseState& mState,
		T dt, U sensitivity,
		glm::highp_u16vec2 resolution = { STANDARD_WIDTH , STANDARD_HEIGHT }) {
		auto vec2 = scaledMouseMovementVec2(mState, dt, sensitivity, resolution);
		return glm::vec3(vec2.x, vec2.y, 0.0f);
	}
};


#endif // INPUTMANAGER_HPP