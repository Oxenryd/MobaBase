#ifndef WINDOWSURFACE_HPP
#define WINDOWSURFACE_HPP

#ifdef BUILD_WIN
    #ifndef _INC_WINAPIFAMILY
        #include <windows.h>
    #endif
#endif

#include "MobaMath.hpp"

#include <cstdint>
#include <glm/glm.hpp>

#include "Log.hpp"
#include "Delegate.hpp"
#include "ErrorCodes.hpp"
#include "Bits.hpp"
#include "InputTypes.h"

KeyCode vkToKeyCode(USHORT vkey, USHORT makeCode, USHORT flags) {
    const bool isE0 = (flags & RI_KEY_E0);
    const bool isE1 = (flags & RI_KEY_E1);  // rarely used

    switch (vkey) {
        case 'A': return KeyCode::A;
        case 'B': return KeyCode::B;
        case 'C': return KeyCode::C;
        case 'D': return KeyCode::D;
        case 'E': return KeyCode::E;
        case 'F': return KeyCode::F;
        case 'G': return KeyCode::G;
        case 'H': return KeyCode::H;
        case 'I': return KeyCode::I;
        case 'J': return KeyCode::J;
        case 'K': return KeyCode::K;
        case 'L': return KeyCode::L;
        case 'M': return KeyCode::M;
        case 'N': return KeyCode::N;
        case 'O': return KeyCode::O;
        case 'P': return KeyCode::P;
        case 'Q': return KeyCode::Q;
        case 'R': return KeyCode::R;
        case 'S': return KeyCode::S;
        case 'T': return KeyCode::T;
        case 'U': return KeyCode::U;
        case 'V': return KeyCode::V;
        case 'W': return KeyCode::W;
        case 'X': return KeyCode::X;
        case 'Y': return KeyCode::Y;
        case 'Z': return KeyCode::Z;

        case '0': return KeyCode::Digit0;
        case '1': return KeyCode::Digit1;
        case '2': return KeyCode::Digit2;
        case '3': return KeyCode::Digit3;
        case '4': return KeyCode::Digit4;
        case '5': return KeyCode::Digit5;
        case '6': return KeyCode::Digit6;
        case '7': return KeyCode::Digit7;
        case '8': return KeyCode::Digit8;
        case '9': return KeyCode::Digit9;

        case VK_ESCAPE: return KeyCode::Escape;
        case VK_TAB: return KeyCode::Tab;
        case VK_CAPITAL: return KeyCode::CapsLock;
        case VK_RETURN: return KeyCode::Enter;
        case VK_SPACE: return KeyCode::Space;
        case VK_BACK: return KeyCode::Backspace;
        case VK_INSERT: return KeyCode::Insert;
        case VK_DELETE: return KeyCode::Delete;
        case VK_HOME: return KeyCode::Home;
        case VK_END: return KeyCode::End;
        case VK_PRIOR: return KeyCode::PageUp;
        case VK_NEXT: return KeyCode::PageDown;
        case VK_LEFT: return KeyCode::ArrowLeft;
        case VK_RIGHT: return KeyCode::ArrowRight;
        case VK_UP: return KeyCode::ArrowUp;
        case VK_DOWN: return KeyCode::ArrowDown;
        case VK_SNAPSHOT: return KeyCode::PrintScreen;
        case VK_SCROLL: return KeyCode::ScrollLock;
        case VK_PAUSE: return KeyCode::PauseBreak;

        case VK_OEM_MINUS: return KeyCode::Minus;
        case VK_OEM_PLUS: return KeyCode::Equals;
        case VK_OEM_4: return KeyCode::LeftBracket;
        case VK_OEM_6: return KeyCode::RightBracket;
        case VK_OEM_5: return KeyCode::Backslash;
        case VK_OEM_1: return KeyCode::Semicolon;
        case VK_OEM_7: return KeyCode::Apostrophe;
        case VK_OEM_3: return KeyCode::Grave;
        case VK_OEM_COMMA: return KeyCode::Comma;
        case VK_OEM_PERIOD: return KeyCode::Period;
        case VK_OEM_2: return KeyCode::Slash;

        case VK_LSHIFT: return KeyCode::LeftShift;
        case VK_RSHIFT: return KeyCode::RightShift;
        case VK_LCONTROL: return KeyCode::LeftControl;
        case VK_RCONTROL: return KeyCode::RightControl;
        case VK_LMENU: return KeyCode::LeftAlt;
        case VK_RMENU: return KeyCode::RightAlt;
        case VK_LWIN: return KeyCode::LeftSuper;
        case VK_RWIN: return KeyCode::RightSuper;

        case VK_NUMPAD0: return KeyCode::Numpad0;
        case VK_NUMPAD1: return KeyCode::Numpad1;
        case VK_NUMPAD2: return KeyCode::Numpad2;
        case VK_NUMPAD3: return KeyCode::Numpad3;
        case VK_NUMPAD4: return KeyCode::Numpad4;
        case VK_NUMPAD5: return KeyCode::Numpad5;
        case VK_NUMPAD6: return KeyCode::Numpad6;
        case VK_NUMPAD7: return KeyCode::Numpad7;
        case VK_NUMPAD8: return KeyCode::Numpad8;
        case VK_NUMPAD9: return KeyCode::Numpad9;
        case VK_MULTIPLY: return KeyCode::NumpadMultiply;
        case VK_ADD: return KeyCode::NumpadAdd;
        case VK_SUBTRACT: return KeyCode::NumpadSubtract;
        case VK_DECIMAL: return KeyCode::NumpadDecimal;
        case VK_DIVIDE: return KeyCode::NumpadDivide;

        case VK_F1: return KeyCode::F1;
        case VK_F2: return KeyCode::F2;
        case VK_F3: return KeyCode::F3;
        case VK_F4: return KeyCode::F4;
        case VK_F5: return KeyCode::F5;
        case VK_F6: return KeyCode::F6;
        case VK_F7: return KeyCode::F7;
        case VK_F8: return KeyCode::F8;
        case VK_F9: return KeyCode::F9;
        case VK_F10: return KeyCode::F10;
        case VK_F11: return KeyCode::F11;
        case VK_F12: return KeyCode::F12;
    }

    return KeyCode::Unknown;
}

KeyEvent mapRawKeyboardEvent(const RAWKEYBOARD& keyboard) {
    const USHORT vkey = keyboard.VKey;
    const USHORT makeCode = keyboard.MakeCode;
    const USHORT flags = keyboard.Flags;

    // Handle generic remapping
    USHORT mappedVKey = vkey;
    bool isE0 = (flags & RI_KEY_E0);
    bool isBreak = (flags & RI_KEY_BREAK);

    if (vkey == VK_SHIFT) {
        mappedVKey = MapVirtualKey(makeCode, MAPVK_VSC_TO_VK_EX);
    } else if (vkey == VK_CONTROL) {
        mappedVKey = isE0 ? VK_RCONTROL : VK_LCONTROL;
    } else if (vkey == VK_MENU) {
        mappedVKey = isE0 ? VK_RMENU : VK_LMENU;
    }

    KeyCode code = vkToKeyCode(mappedVKey, makeCode, flags);

    KeyAction action = isBreak ? KeyAction::Release : KeyAction::Press;

    // Use GetAsyncKeyState to check if modifier keys are currently down
    //bool shiftDown = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) ||
    //    (GetAsyncKeyState(VK_RSHIFT) & 0x8000);
    //bool ctrlDown = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) ||
    //    (GetAsyncKeyState(VK_RCONTROL) & 0x8000);
    //bool altDown = (GetAsyncKeyState(VK_LMENU) & 0x8000) ||
    //    (GetAsyncKeyState(VK_RMENU) & 0x8000);

    return KeyEvent{
        .code = code,
        .action = action
    };
}



//struct RawKeyEvent
//{
//    USHORT virtualKey;
//    USHORT scanCode;
//    bool isPressed;
//    bool isExtended;
//};

enum class MouseButtonDownState
{
#ifdef BUILD_WIN
    None        = 0,
    Left        = 0x0001,
    Right       = 0x0002,
    ShiftKey    = 0x0004,
    ControlKey  = 0x0008,
    Middle      = 0x0010,
    XButton1    = 0x0020,
    XButton2    = 0x0040
#endif
};

struct WindowMouseState
{
    glm::i16vec2 relativePosition;
    glm::i16vec2 wheel;
    SizedBitField<uint8_t> buttonStates;
};


struct WindowSurface
{
    union alignas (4) KeysBitfield
    {
        KeysBitfield(uint32_t field) : value{ field } {}
        KeysBitfield& operator=(uint32_t field) {
            value = field;
            return *this;
        }
        uint32_t value;
        struct
        {
            uint32_t RepeatCount        : 16;
            uint32_t ScanCode           : 8; 
            uint32_t ExtendedKey        : 1; // 1 = Extended, right ALT, right CTRL
            uint32_t Reserved1          : 4; //
            uint32_t ContextCode        : 1; // ALT pressed
            uint32_t PreviousKeyState   : 1; // 1 = was down
            uint32_t TransitionState    : 1; // 0 = key down, 1 = key up
        };
    };

    enum class SizeType : uint8_t
    {
        Restored = 0,
        Minimized = 1,
        Maximized = 2,
        MaxShow = 3,
        MaxHide = 4
    };

    enum class ActivateMode : uint8_t
    {
        LostFocus,
        GotFocus,
        GotFocusClick
    };

    enum class MinState
    {
        NotMinimized,
        Minimized
    };

    uint16_t width = static_cast<uint16_t>(-1);
    uint16_t height = static_cast<uint16_t>(-1);
    std::string appName;

    Event<> onCreate;
    Event<> onDestroy;
    Event<> onClose;    
    Event<SizeType, glm::u16vec2> onResize;
    Event<UINT, UINT> onMove;
    Event<ActivateMode, MinState> onActivate;
    Event<> onGotFocus;
    Event<> onLostFocus;
    Event<KeyEvent> onKeyEvent;
    Event<uint8_t, KeysBitfield> onKeyDown;
    Event<uint8_t, KeysBitfield> onKeyUp;
    Event<uint64_t> onChar;
    Event<WindowMouseState> onMouse;
    Event<> onSysCommand;
    Event<uint16_t, IntRect<32>> onDpiChanged;
    Event<> onQuit;
    Event<WindowSurface* const, HRAWINPUT> onRawInput;
    
    WindowSurface() = default;
    ~WindowSurface() {}
#ifdef BUILD_WIN
    
    WindowSurface(const WindowSurface& other) : 
        windowHandle{ other.windowHandle }
    {}
    WindowSurface& operator=(const WindowSurface& rhs) {
        windowHandle = rhs.windowHandle;
        return *this;
    }

    HWND windowHandle = nullptr;
    HINSTANCE windowInstance = nullptr;
    void showWindow(int showCommand) { 
        ShowWindow(windowHandle, showCommand);
    }
    void destroyWindow() { 
        DestroyWindow(windowHandle);
    }

    ErrorCode enableRawInput() {
        // Enable raw input
        //RAWINPUTDEVICE rid;
        //rid.usUsagePage = 0x01; // Generic desktop controls
        //rid.usUsage = 0x02;     // Mouse
        //rid.dwFlags = RIDEV_INPUTSINK;        // Default: receive input when app is focused
        //rid.hwndTarget = windowHandle;

        RAWINPUTDEVICE rid[2];

        // Mouse
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02;
        rid[0].dwFlags = RIDEV_INPUTSINK;
        rid[0].hwndTarget = windowHandle;

        // Keyboard
        rid[1].usUsagePage = 0x01;
        rid[1].usUsage = 0x06;  // <- keyboard
        rid[1].dwFlags = RIDEV_INPUTSINK;
        rid[1].hwndTarget = windowHandle;



        if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
            DWORD err = GetLastError();
            return static_cast<ErrorCode>(err);
        }

        return ErrorCode::OK;
    }




#endif // BUILD_WIN
};


#ifdef BUILD_WIN

static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    WindowSurface* surface = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        surface = reinterpret_cast<WindowSurface*>(create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)surface);
        if (surface)
            surface->onCreate.notify();
    } else {
        surface = reinterpret_cast<WindowSurface*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (surface) {
        
        switch (uMsg) {
            case WM_DESTROY:
            {
                surface->onDestroy.notify();
            } break;

            case WM_CLOSE:
            {
                surface->onClose.notify();
            } break;

            case WM_SIZE:
            {
                auto type = static_cast<WindowSurface::SizeType>(wParam);
                uint16_t newWidth = LOWORD(lParam);
                uint16_t newHeight = HIWORD(lParam);
                surface->onResize.notify(type, glm::u16vec2{ newWidth,newHeight });
            } break;

            case WM_MOVE:
            {
                UINT newX = LOWORD(lParam);
                UINT newY = HIWORD(lParam);
                surface->onMove.notify(newX, newY);
            } break;

            case WM_ACTIVATE:
            {
                auto mode = static_cast<WindowSurface::ActivateMode>(LOWORD(wParam));
                auto minState = HIWORD(wParam) != 0
                    ? WindowSurface::MinState::Minimized
                    : WindowSurface::MinState::NotMinimized;
                surface->onActivate.notify(mode, minState);             
            } break;

            case WM_SETFOCUS:
                surface->onGotFocus.notify();
                break;

            case WM_KILLFOCUS:
                surface->onLostFocus.notify();
                break;


            case WM_KEYDOWN:
            {
                auto code = static_cast<uint8_t>(wParam);
                auto bitfield = static_cast<WindowSurface::KeysBitfield>(lParam);
                surface->onKeyDown.notify(code, bitfield);
            } break;

            case WM_KEYUP:
            {
                auto code = static_cast<uint8_t>(wParam);
                auto bitfield = static_cast<WindowSurface::KeysBitfield>(lParam);
                surface->onKeyUp.notify(code, bitfield);
            } break;

            case WM_CHAR:
            {
                auto unicode = static_cast<uint16_t>(wParam);
                surface->onChar.notify(unicode);
            } break;

            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEMOVE:
            {
                auto posX = static_cast<int16_t>(LOWORD(lParam));
                auto posY = static_cast<int16_t>(HIWORD(lParam));
                auto btnState = static_cast<uint8_t>(LOWORD(wParam));
                WindowMouseState state{};
                state.relativePosition = { posX, posY };
                state.buttonStates.copyField(btnState);
                surface->onMouse.notify(state);
            } break;

            case WM_MOUSEWHEEL:
            {
                auto posX = static_cast<int16_t>(LOWORD(lParam));
                auto posY = static_cast<int16_t>(HIWORD(lParam));
                auto btnState = static_cast<uint8_t>(LOWORD(wParam));
                auto wheel = static_cast<int16_t>(HIWORD(wParam));
                WindowMouseState state{};
                state.relativePosition = { posX, posY };
                state.buttonStates.copyField(btnState);
                state.wheel = glm::i16vec2{ state.wheel.x, wheel };
                surface->onMouse.notify(state);
            } break;

            case WM_MOUSEHWHEEL:
            {
                auto posX = static_cast<int16_t>(LOWORD(lParam));
                auto posY = static_cast<int16_t>(HIWORD(lParam));
                auto btnState = static_cast<uint8_t>(LOWORD(wParam));
                auto wheel = static_cast<int16_t>(HIWORD(wParam));
                WindowMouseState state{};
                state.relativePosition = { posX, posY };
                state.buttonStates.copyField(btnState);
                state.wheel = glm::i16vec2{ wheel, state.wheel.y };
                surface->onMouse.notify(state);
            } break;

            case WM_QUIT:
            {
                surface->onQuit.notify();
            } break;

            case WM_DPICHANGED:
            {
                RECT* rp = reinterpret_cast<RECT*>(lParam);
                int16_t dpi = static_cast<int16_t>(HIWORD(wParam));
                IntRect<32> rect{ rp->left, rp->top, rp->right - rp->left, rp->bottom - rp->top };
                surface->onDpiChanged.notify(dpi, rect);
            } break;

            case WM_INPUT:
                surface->onRawInput.notify(surface, (HRAWINPUT)lParam);
                break;

            default:
                break;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static ErrorCode createSurface(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow,
    LPCWSTR className,
    LPCWSTR windowTitle,
    uint16_t width, uint16_t height,
    WindowSurface* surface)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    ATOM atom = RegisterClassExW(&wc);

    if (atom == 0) {
        DWORD err = GetLastError();
        MessageBoxW(nullptr, L"RegisterClassW failed", L"Error", MB_OK);
        return static_cast<ErrorCode>(err);
    }

    HWND g_hWnd = CreateWindowExW(
        0,
        className,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, surface
    );

    if (!g_hWnd) {
        DWORD err = GetLastError();
        return static_cast<ErrorCode>(err);
    }

    LONG style = GetWindowLong(g_hWnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    style |= WS_MINIMIZEBOX;
    SetWindowLong(g_hWnd, GWL_STYLE, style);

    surface->windowHandle = g_hWnd;
    surface->windowInstance = hInstance;
    surface->width = width;
    surface->height = height;

    SetWindowTextW(g_hWnd, windowTitle);

    return ErrorCode::OK;
}

INLINE static void HandleRawInputWin32(WindowSurface* const surface, HRAWINPUT hRawInput) {
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
    } else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        const RAWKEYBOARD& keyboard = raw->data.keyboard;

        KeyEvent event = mapRawKeyboardEvent(keyboard);
        surface->onKeyEvent.notify(event);
    }
}

#endif // BUILD_WIN

#endif // WINDOWSURFACE_HPP