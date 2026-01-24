#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <glm/glm.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
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

enum class InputType : uint8_t
{
	Keyboard,
	Mouse,
	Gamepad,
	Custom
};

enum class BindingType : uint8_t
{
	KeyButton,
	Vector1D,
	Vector2D,
};

struct alignas(8) InputBinding
{
	uint32_t raw{0};
	uint16_t id{0};
	InputType inputType{InputType::Keyboard};
	BindingType bindType{BindingType::KeyButton};
};



class Action;
struct alignas(8) InputData
{
private:
	friend Action;
	unsigned char rawData[8]{};
	void setMouseState(MouseState* statePtr) {
		auto& ptr = reinterpret_cast<MouseState*&>(rawData);
		ptr = statePtr;
	}
	void setVector1D(const float value) {
		*reinterpret_cast<float*>(rawData) = value;
	}
	void setVector2D(const glm::vec2& value) {
		std::construct_at<glm::vec2>(reinterpret_cast<glm::vec2*>(rawData), value);
	}
	void setRawKeyCode(const uint16_t code) {
		*reinterpret_cast<uint16_t*>(rawData) = code;
	}

public:
	const MouseState& getMouseState() const {return *reinterpret_cast<const MouseState*>(rawData);}
	float getVector1D() const { return *reinterpret_cast<const float*>(rawData); }
	const glm::vec2& getVector2D() const { return *reinterpret_cast<const glm::vec2*>(rawData); }
	uint16_t getRawKeyCode() const { return *reinterpret_cast<const uint16_t*>(rawData); }
};




class Action {
	std::string m_name;
	std::vector<InputBinding> m_bindings;


public:
	Event<ActionState> onTrigger;
	std::string& name() { return m_name; }
	const std::vector<InputBinding>& bindings() const { return m_bindings; }
	std::vector<InputBinding>& bindings() { return m_bindings; }

	void trigger(const ActionState state) const {
		onTrigger.notify(state);
	}
};




class InputManager
{
	inline static InputManager* s_instance = nullptr;
	boost::unordered_flat_map<KeyCombo, std::vector<Action>, KeyComboHash> m_keyboardActions;
	boost::unordered_flat_map<KeyCombo, std::vector<Event<ActionState>*>, KeyComboHash> m_keyDownActionMap;
	boost::unordered_flat_set<int, IntHash<int>> m_keysDown;

	std::vector<Action> m_mouseActions;
	std::vector<Action> m_gamepadActions;


	uint8_t m_mButtonsDown = 0;
	bool m_ignoreNextMouseDelta = false;
	MouseState m_lastMouseState;
	MouseState m_currentMouseState;
	glm::i16vec2 m_lastDeltaMax{};


	bool m_gotMousePosThisFrame = false;
	float m_deltaTime{};


	void _notifyMouseButtonEvent(const uint8_t mask, const ActionState action) const {

		switch (mask) {

			default: return;

			case static_cast<uint8_t>(MButton::Button1): switch (action) {
				default: return;
				case ActionState::Started: onMouseLeftDown.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseLeftHold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseLeftUp.notify(m_currentMouseState); return;
			}

			case static_cast<uint8_t>(MButton::Button2): switch (action) {
				default: return;
				case ActionState::Started: onMouseRightDown.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseRightHold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseRightUp.notify(m_currentMouseState); return;
			}

			case static_cast<uint8_t>(MButton::Button3): switch (action) {
				default: return;
				case ActionState::Started: onMouseMiddleDown.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseMiddleHold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseMiddleUp.notify(m_currentMouseState); return;
			}

			case static_cast<uint8_t>(MButton::Button4): switch (action) {
				default: return;
				case ActionState::Started: onMouseM4Down.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseM4Hold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseM4Up.notify(m_currentMouseState); return;
			}

			case static_cast<uint8_t>(MButton::Button5): switch (action) {
				default: return;
				case ActionState::Started: onMouseM5Down.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseM5Hold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseM5Up.notify(m_currentMouseState); return;
			}

			case static_cast<uint8_t>(MButton::Button6): switch (action) {
				default: return;
				case ActionState::Started: onMouseM6Down.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseM6Hold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseM6Up.notify(m_currentMouseState); return;
			}

			case static_cast<uint8_t>(MButton::Button7): switch (action) {
				default: return;
				case ActionState::Started: onMouseM7Down.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseM7Hold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseM7Up.notify(m_currentMouseState); return;
			}

			case static_cast<uint8_t>(MButton::Button8): switch (action) {
				default: return;
				case ActionState::Started: onMouseM8Down.notify(m_currentMouseState); return;
				case ActionState::Performed: onMouseM8Hold.notify(m_lastMouseState); return;
				case ActionState::Stopped: onMouseM8Up.notify(m_currentMouseState); return;
			}

		}

	}


	static void key_callback(GLFWwindow* window, int key, int scancode, int glfwAction, int mods)
	{
		const KeyCombo combo{key, mods};
		const auto aState = static_cast<ActionState>(glfwAction);
		const auto it = s_instance->m_keyboardActions.find(combo);
		if (it != s_instance->m_keyboardActions.end()) {
			const auto& actionList = it->second;

			for(const auto& action : it->second) {

				switch (aState) {
					default: break;

					case ActionState::Started: {
						const auto keyDownIT = s_instance->m_keyDownActionMap.find(combo);
						if (keyDownIT == s_instance->m_keyDownActionMap.end()) {
							const auto [iter, snd] =
								s_instance->m_keyDownActionMap.emplace(combo, std::vector<Event<ActionState>*>());
							auto* ptr = const_cast<Event<ActionState>*>(&action.onTrigger);
							iter->second.push_back(ptr);
						}
					} break;

					case ActionState::Stopped: {
						const auto keyDownIT = s_instance->m_keyDownActionMap.find(combo);
						if (keyDownIT != s_instance->m_keyDownActionMap.end()) {
							s_instance->m_keyDownActionMap.erase(keyDownIT);
						}

					} break;
				}

				action.trigger(aState);
			}
		}

		switch (aState) {
			default: break;

			case ActionState::Started: {
				const auto keyIt = s_instance->m_keysDown.find(key);
				if (keyIt == s_instance->m_keysDown.end()) {
					s_instance->m_keysDown.insert(key);
					s_instance->onKeyDown.notify(glfwToKeyCode(key));
				}
			} break;
			case ActionState::Stopped: {
				const auto keyIt = s_instance->m_keysDown.find(key);
				if (keyIt != s_instance->m_keysDown.end()) {
					s_instance->m_keysDown.erase(keyIt);
					s_instance->onKeyUp.notify(glfwToKeyCode(key));
				}
			} break;
		}
	}


public:
    ~InputManager() {}
	explicit InputManager(WindowContext* context)
	{
    	if (s_instance != nullptr) {
    		LOGLINE(LogType::Error, LogMod::Input, "There Can only be one InputManager!!");
    		throw std::runtime_error("There Can only be one InputManager!!");
    	}
    	s_instance = this;
    	glfwSetKeyCallback(context->window(), key_callback);
	}

	INLINE void update(const double dt) {

		m_deltaTime = static_cast<float>(dt);
		//m_lastDeltaMax = { 0,0 };
		//m_currentMouseState.lastPositionDelta = m_lastDeltaMax;
		m_currentMouseState.lastPositionDelta = glm::ivec2{ 0, 0 };


		for (auto& actionList: m_keyDownActionMap | std::views::values) {
			for (const auto& actionEvent : actionList ) {
				actionEvent->notify(ActionState::Stopped);
			}
		}
    	for (const auto& key : m_keysDown) {
    		onKeyHold.notify(glfwToKeyCode(key));
    	}

		for (uint8_t i = 0; i < 8; ++i) {
			const uint8_t mask = 1 << i;
			if (m_mButtonsDown & mask)
				_notifyMouseButtonEvent(mask, ActionState::Performed);
		}


		// MouseButtons
		//uint8_t last = m_lastMouseState.buttonState.getField();
		const uint8_t curr = m_currentMouseState.buttonState.raw;
		for (uint8_t i = 0; i < 8; ++i) {
			const uint8_t mask = 1 << i;
			if (m_mButtonsDown & mask) {
				if (!(curr & mask))
					_notifyMouseButtonEvent(mask, ActionState::Stopped);
			} else if (curr & mask)
				_notifyMouseButtonEvent(mask, ActionState::Started);
		}
		m_mButtonsDown = curr;
		m_lastMouseState = m_currentMouseState;
	}

	MouseState& currentMouseState() const { return const_cast<MouseState&>(m_currentMouseState); }
	MouseState& lastMouseState() const { return const_cast<MouseState&>(m_lastMouseState); }

	// void mouseToWindowCenter() {
	// 	m_ignoreNextMouseDelta = true;
	// 	POINT pt{ m_ws->width / 2, m_ws->height / 2 };
	// 	ClientToScreen(m_ws->windowHandle, &pt);
	// 	SetCursorPos(pt.x, pt.y);
	// }
	//
	// void enableRelativeMouse() {
	// 	// Hide cursor
	// 	while (ShowCursor(FALSE) >= 0); // ensure hidden
	//
	// 	// Confine to client area
	// 	RECT rect;
	// 	GetClientRect(m_ws->windowHandle, &rect);
	// 	POINT tl = { rect.left, rect.top };
	// 	POINT br = { rect.right, rect.bottom };
	// 	ClientToScreen(m_ws->windowHandle, &tl);
	// 	ClientToScreen(m_ws->windowHandle, &br);
	// 	RECT clipRect = { tl.x, tl.y, br.x, br.y };
	// 	ClipCursor(&clipRect);
	// }
	//
	// void disableRelativeMouse() {
	// 	// Show cursor
	// 	while (ShowCursor(TRUE) < 0); // ensure visible
	//
	// 	// Release cursor
	// 	ClipCursor(nullptr);
	// }

	// static void handleKeyEvent(InputManager* _this, KeyCombo event) {
	//
	// 	_this->onKeyEvent.notify(event); // for manual handling
	//
	// 	const auto it = _this->m_keyButtonDownMap.find(static_cast<uint16_t>(event.code));
	// 	if (event.action == ActionState::Stopped) {
	// 		if (it != _this->m_keyButtonDownMap.end()) {
	// 			_this->m_keyButtonDownMap.erase(it);
	// 			_this->onKeyUp.notify(event.code);
	// 		}
	// 	} else {
	// 		if (it == _this->m_keyButtonDownMap.end()) {
	// 			_this->m_keyButtonDownMap.insert(static_cast<uint16_t>(event.code));
	// 			_this->onKeyDown.notify(event.code);
	// 		}
	// 	}
	// }

	// EVENTS




	// Keyboard
	Event<const KeyCombo> onKeyEvent;
	Event<const KeyCode> onKeyDown;
	Event<const KeyCode> onKeyHold;
	Event<const KeyCode> onKeyUp;


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
	Event<MouseState> onMouseM6Down;
	Event<MouseState> onMouseM6Hold;
	Event<MouseState> onMouseM6Up;
	Event<MouseState> onMouseM7Down;
	Event<MouseState> onMouseM7Hold;
	Event<MouseState> onMouseM7Up;
	Event<MouseState> onMouseM8Down;
	Event<MouseState> onMouseM8Hold;
	Event<MouseState> onMouseM8Up;
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