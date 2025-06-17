#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include "WindowSurface.h"

static void handleRawInput(HRAWINPUT hRawInput) {
    UINT dataSize = 0;

    // First, query required size
    GetRawInputData(hRawInput, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));

    std::vector<BYTE> rawData(dataSize);

    if (GetRawInputData(hRawInput, RID_INPUT, rawData.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize) {
        // Handle error here
        return;
    }

    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawData.data());

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        const RAWMOUSE& mouse = raw->data.mouse;

        if (mouse.usFlags == MOUSE_MOVE_RELATIVE) {
            LONG deltaX = mouse.lLastX;
            LONG deltaY = mouse.lLastY;

            // This is your raw high-precision delta:
            // deltaX = horizontal movement since last WM_INPUT
            // deltaY = vertical movement since last WM_INPUT

            // Dispatch into your InputManager, etc:
            //handleRawMouseDelta(deltaX, deltaY);
        }

        // Optional: handle raw mouse buttons
        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) { /* handle left down */ }
        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) { /* handle left up */ }
        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) { /* handle right down */ }
        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) { /* handle right up */ }
    }
}

class InputManager
{
private:
	WindowSurface* m_ws;
public:
    ~InputManager() {}
	InputManager(WindowSurface* surface) :
		m_ws{surface} {
	
        m_ws->onRawInput.subscribe(&handleRawInput);
	}

	void pollEvents() {
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
};



#endif // INPUTMANAGER_HPP