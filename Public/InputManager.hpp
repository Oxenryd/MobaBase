#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <glm/glm.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
//#include <oneapi/tbb/detail/_task.h>

#include "WindowContext.h"
#include "Delegate.hpp"
#include "InputTypes.h"
//#include "../out/build/Linux-x64-debug/_deps/glfw-src/src/wl_platform.h"


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

enum class CursorMode
{
	Normal = GLFW_CURSOR,
	Hidden = GLFW_CURSOR_HIDDEN,
	HiddenGrabbed = GLFW_CURSOR_DISABLED
};

class Engine;
class InputManager
{
	friend Engine;
	inline static InputManager* s_instance = nullptr;

	boost::unordered_flat_map<
		KeyCombo, std::vector<Action>, KeyComboHash> m_keyboardActions;
	boost::unordered_flat_map<
		KeyCombo, std::vector<Event<ActionState>*>, KeyComboHash> m_keyDownActionMap;
	boost::unordered_flat_set<
		int, IntHash<int>> m_keysDown;

	boost::unordered_flat_map<
		MouseButtonState, std::vector<Action>, MouseButtonStateHash> m_mouseButtonActions;
	boost::unordered_flat_map<
		MouseButtonState, std::vector<Event<ActionState>*>, MouseButtonStateHash> m_mouseDownActionMap;
	boost::unordered_flat_set<
		int, IntHash<int>> m_mouseButtonsDown;


	MouseState m_lastMouseState;
	MouseState m_currentMouseState;

	GLFWwindow* m_window;
	float m_deltaTime{};

	// Keyboard
	Event<const KeyCombo> m_onKeyEvent;
	Event<const KeyCode> m_onKeyDown;
	Event<const KeyCode> m_onKeyHold;
	Event<const KeyCode> m_onKeyUp;

	// Mouse
	Event<const MouseState&, const MouseButton> m_onMouseButtonDown;
	Event<const MouseState&, const MouseButton> m_onMouseButtonHold;
	Event<const MouseState&, const MouseButton> m_onMouseButtonUp;
	Event<const MouseState&, const glm::i16vec2> m_onMouseMove;

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
		if (window != s_instance->m_window)
			return;

		const auto bState = MouseButtonState(button, mods);
		const auto aState = static_cast<ActionState>(action);
		const auto it = s_instance->m_mouseButtonActions.find(bState);
		if (it != s_instance->m_mouseButtonActions.end()) {
			//TODO
		}

		switch (aState) {
			default: break;

			case ActionState::Started: {
				const auto mDownIT = s_instance->m_mouseButtonsDown.find(button);
				if (mDownIT == s_instance->m_mouseButtonsDown.end()) {
					s_instance->m_mouseButtonsDown.insert(button);
				}
				s_instance->m_currentMouseState.buttonState =
					s_instance->m_currentMouseState.buttonState | bState;
				s_instance->m_onMouseButtonDown.notify(s_instance->m_currentMouseState, glfwToMButton(button));
			} break;

			case ActionState::Stopped: {
				const auto mDownIT = s_instance->m_mouseButtonsDown.find(button);
				if (mDownIT != s_instance->m_mouseButtonsDown.end()) {
					s_instance->m_mouseButtonsDown.erase(button);
				}
				s_instance->m_currentMouseState.buttonState =
					s_instance->m_currentMouseState.buttonState & ~bState;
				s_instance->m_onMouseButtonUp.notify(s_instance->m_currentMouseState, glfwToMButton(button));
			} break;
		}
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int glfwAction, int mods)
	{
		if (window != s_instance->m_window)
			return;

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
					s_instance->m_onKeyDown.notify(glfwToKeyCode(key));
				}
			} break;
			case ActionState::Stopped: {
				const auto keyIt = s_instance->m_keysDown.find(key);
				if (keyIt != s_instance->m_keysDown.end()) {
					s_instance->m_keysDown.erase(keyIt);
					s_instance->m_onKeyUp.notify(glfwToKeyCode(key));
				}
			} break;
		}
	}

	INLINE void update(const double dt) {

		m_deltaTime = static_cast<float>(dt);

		// Mouse input
		double xpos, ypos;
		glfwGetCursorPos(m_window, &xpos, &ypos);
		s_instance->m_currentMouseState.relativePosition = glm::vec2(xpos, ypos);
		s_instance->m_currentMouseState.absoluteScreenPosition = glm::vec2(xpos, ypos);
		m_currentMouseState.deltaPosition =
			m_currentMouseState.relativePosition - m_lastMouseState.relativePosition;
		if (m_currentMouseState.deltaPosition.x != 0 || m_currentMouseState.deltaPosition.y != 0) {
			m_onMouseMove.notify(m_currentMouseState, m_currentMouseState.deltaPosition);
		}
		for (const auto& btn : m_mouseButtonsDown) {
			m_onMouseButtonHold.notify(m_currentMouseState, glfwToMButton(btn));
		}



		// Keyboard input
		for (auto& actionList: m_keyDownActionMap | std::views::values) {
			for (const auto& actionEvent : actionList ) {
				actionEvent->notify(ActionState::Stopped);
			}
		}
		for (const auto& key : m_keysDown) {
			m_onKeyHold.notify(glfwToKeyCode(key));
		}

		m_lastMouseState = m_currentMouseState;
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
    	m_window = context->window();

    	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    	glfwSetKeyCallback(m_window, key_callback);
    	glfwSetMouseButtonCallback(m_window, mouse_button_callback);

	}

	INLINE static void setCursorMode(const CursorMode mode) {
	    switch (mode) {
		    case CursorMode::Normal:
	    		glfwSetInputMode(s_instance->m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	    		glfwSetInputMode(s_instance->m_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
	    		break;

	    	case CursorMode::Hidden:
	    		glfwSetInputMode(s_instance->m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	    		glfwSetInputMode(s_instance->m_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
	    		break;

	    	case CursorMode::HiddenGrabbed:
	    		glfwSetInputMode(s_instance->m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	    		glfwSetInputMode(s_instance->m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	    		break;
	    }
    }



	static const MouseState& currentMouseState() { return s_instance->m_currentMouseState; }
	static const MouseState& lastMouseState() { return s_instance->m_lastMouseState; }
	static glm::vec3 deltaPosVec3_swizzled() {
    	const auto delta = s_instance->m_currentMouseState.deltaPosition;
	    return glm::vec3(delta.x, delta.y, 0.0f);
    }

	// EVENTS




	// Keyboard
	static Event<const KeyCombo>& onKeyEvent() { return s_instance->m_onKeyEvent; }
	static Event<const KeyCode>& onKeyDown() { return s_instance->m_onKeyDown; }
	static Event<const KeyCode>& onKeyUp() { return s_instance->m_onKeyUp; }
	static Event<const KeyCode>& onKeyHold() { return s_instance->m_onKeyHold; }


	// Mouse
	static Event<const MouseState&, const MouseButton>& onMouseDown() {
	    return s_instance->m_onMouseButtonDown; }
	static Event<const MouseState&, const MouseButton>& onMouseUp() {
    	return s_instance->m_onMouseButtonUp; }
	static Event<const MouseState&, const MouseButton>& onMouseHold() {
    	return s_instance->m_onMouseButtonHold; }
	static Event<const MouseState&, const glm::i16vec2>& onMouseMove() {
    	return s_instance->m_onMouseMove; }

};


// class Input
// {
// private:
// 	static constexpr uint16_t STANDARD_WIDTH = 1280;
// 	static constexpr uint16_t STANDARD_HEIGHT = 800;
// public:
//
// 	template <std::floating_point T, std::floating_point U>
// 	INLINE static glm::vec2 scaledMouseMovementVec2(
// 		const MouseState& mState,
// 		T dt, U sensitivity,
// 		glm::highp_u16vec2 resolution = { STANDARD_WIDTH , STANDARD_HEIGHT }) {
//
// 		//auto rec = 0.00025f / dt;
// 		glm::vec2 scaledRes = glm::vec2{ static_cast<float>(resolution.x), static_cast<float>(resolution.y) } * INPUT_RESO_SCALE_FACTOR;
// 		float sense = static_cast<float>(sensitivity * sensitivity);
// 		return glm::vec2{
// 			-(float)mState.deltaPosition.x * sense * scaledRes.x,
// 			-(float)mState.deltaPosition.y * sense * scaledRes.y
// 		};
// 	}
// 	template <std::floating_point T, std::floating_point U>
// 	INLINE static glm::vec3 scaledMouseMovementVec3(
// 		const MouseState& mState,
// 		T dt, U sensitivity,
// 		glm::highp_u16vec2 resolution = { STANDARD_WIDTH , STANDARD_HEIGHT }) {
// 		auto vec2 = scaledMouseMovementVec2(mState, dt, sensitivity, resolution);
// 		return glm::vec3(vec2.x, vec2.y, 0.0f);
// 	}
// };


#endif // INPUTMANAGER_HPP