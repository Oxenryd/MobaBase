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
    Event<uint8_t, KeysBitfield> onKeyDown;
    Event<uint8_t, KeysBitfield> onKeyUp;
    Event<uint64_t> onChar;
    Event<WindowMouseState> onMouse;
    Event<> onSysCommand;
    Event<uint16_t, IntRect<32>> onDpiChanged;
    Event<> onQuit;
    Event<HRAWINPUT> onRawInput;
    
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
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01; // Generic desktop controls
        rid.usUsage = 0x02;     // Mouse
        rid.dwFlags = RIDEV_INPUTSINK;        // Default: receive input when app is focused
        rid.hwndTarget = windowHandle;

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
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
                surface->onRawInput.notify((HRAWINPUT)lParam);
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

#endif // BUILD_WIN

#endif // WINDOWSURFACE_HPP