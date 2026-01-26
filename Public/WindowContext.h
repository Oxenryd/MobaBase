#ifndef WINDOWSURFACE_HPP
#define WINDOWSURFACE_HPP
#include "ErrorCodes.hpp"

//#ifdef BUILD_WIN
//    #ifndef _INC_WINAPIFAMILY
//        #include <windows.h>
//    #endif
//#endif
//
//
//#ifdef BUILD_WIN
//
//#include "MMath.hpp"
//#include <cstdint>
//#include <glm/glm.hpp>
//
//#include "Log.hpp"
//#include "Delegate.hpp"
//#include "ErrorCodes.hpp"
//#include "Bits.hpp"
//#include "InputTypes.h"
//
//struct MouseDataRaw
//{
//    int deltaX;
//    int deltaY;
//    int absX;
//    int absY;
//};
//
//struct WindowSurface
//{
//    union alignas (4) KeysBitfield
//    {
//        explicit KeysBitfield(const uint32_t field) : value{ field } {}
//        KeysBitfield& operator=(const uint32_t field) {
//            value = field;
//            return *this;
//        }
//        uint32_t value;
//        struct
//        {
//            uint32_t RepeatCount        : 16;
//            uint32_t ScanCode           : 8; 
//            uint32_t ExtendedKey        : 1; // 1 = Extended, right ALT, right CTRL
//            uint32_t Reserved1          : 4; //
//            uint32_t ContextCode        : 1; // ALT pressed
//            uint32_t PreviousKeyState   : 1; // 1 = was down
//            uint32_t TransitionState    : 1; // 0 = key down, 1 = key up
//        };
//    };
//
//    enum class SizeType : uint8_t
//    {
//        Restored = 0,
//        Minimized = 1,
//        Maximized = 2,
//        MaxShow = 3,
//        MaxHide = 4
//    };
//
//    enum class ActivateMode : uint8_t
//    {
//        LostFocus,
//        GotFocus,
//        GotFocusClick
//    };
//
//    enum class MinState
//    {
//        NotMinimized,
//        Minimized
//    };
//
//    uint16_t width = static_cast<uint16_t>(-1);
//    uint16_t height = static_cast<uint16_t>(-1);
//    std::string appName;
//
//    Event<> onCreate;
//    Event<> onDestroy;
//    Event<> onClose;    
//    Event<SizeType, glm::u16vec2> onResize;
//    Event<UINT, UINT> onMove;
//    Event<ActivateMode, MinState> onActivate;
//    Event<> onGotFocus;
//    Event<> onLostFocus;
//    Event<KeyEvent> onKeyEvent;
//    Event<uint8_t, KeysBitfield> onKeyDown;
//    Event<uint8_t, KeysBitfield> onKeyUp;
//    Event<uint64_t> onChar;
//    Event<MouseState> onMouseWheel;
//    Event<MouseState> onMouseButton;
//    Event<MouseDataRaw> onMouseRaw;
//    Event<> onSysCommand;
//    Event<uint16_t, IntRect<32>> onDpiChanged;
//    Event<> onQuit;
//    Event<WindowSurface* const, HRAWINPUT> onRawInput;
//    
//
//
//    WindowSurface() = default;
//    ~WindowSurface() {}
//    
//    WindowSurface(const WindowSurface& other) : 
//        windowHandle{ other.windowHandle }
//    {}
//    WindowSurface& operator=(const WindowSurface& rhs) {
//        windowHandle = rhs.windowHandle;
//        return *this;
//    }
//
//    HWND windowHandle = nullptr;
//    HINSTANCE windowInstance = nullptr;
//    void showWindow(int showCommand) { 
//        ShowWindow(windowHandle, showCommand);
//    }
//    void destroyWindow() { 
//        DestroyWindow(windowHandle);
//    }
//
//    ErrorCode enableRawInput() {
//        // Enable raw input
//        //RAWINPUTDEVICE rid;
//        //rid.usUsagePage = 0x01; // Generic desktop controls
//        //rid.usUsage = 0x02;     // Mouse
//        //rid.dwFlags = RIDEV_INPUTSINK;        // Default: receive input when app is focused
//        //rid.hwndTarget = windowHandle;
//
//        RAWINPUTDEVICE rid[2];
//
//        // Mouse
//        rid[0].usUsagePage = 0x01;
//        rid[0].usUsage = 0x02;
//        rid[0].dwFlags = RIDEV_INPUTSINK;
//        rid[0].hwndTarget = windowHandle;
//
//        // Keyboard
//        rid[1].usUsagePage = 0x01;
//        rid[1].usUsage = 0x06;  // <- keyboard
//        rid[1].dwFlags = 0;//RIDEV_INPUTSINK;
//        rid[1].hwndTarget = windowHandle;
//
//
//
//        if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
//            DWORD err = GetLastError();
//            return static_cast<ErrorCode>(err);
//        }
//
//        return ErrorCode::OK;
//    }
//
//};
//
//
//
//
//static LRESULT CALLBACK WindowProc(
//    HWND hwnd,
//    UINT uMsg,
//    WPARAM wParam,
//    LPARAM lParam)
//{
//    WindowSurface* surface = nullptr;
//
//    if (uMsg == WM_NCCREATE) {
//        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
//        surface = reinterpret_cast<WindowSurface*>(create->lpCreateParams);
//        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)surface);
//        if (surface)
//            surface->onCreate.notify();
//    } else {
//        surface = reinterpret_cast<WindowSurface*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
//    }
//
//    if (surface) {
//        
//        switch (uMsg) {
//            case WM_DESTROY:
//            {
//                surface->onDestroy.notify();
//            } break;
//
//            case WM_CLOSE:
//            {
//                surface->onClose.notify();
//            } break;
//
//            case WM_SIZE:
//            {
//                auto type = static_cast<WindowSurface::SizeType>(wParam);
//                uint16_t newWidth = LOWORD(lParam);
//                uint16_t newHeight = HIWORD(lParam);
//                surface->onResize.notify(type, glm::u16vec2{ newWidth,newHeight });
//            } break;
//
//            case WM_MOVE:
//            {
//                UINT newX = LOWORD(lParam);
//                UINT newY = HIWORD(lParam);
//                surface->onMove.notify(newX, newY);
//            } break;
//
//            case WM_ACTIVATE:
//            {
//                auto mode = static_cast<WindowSurface::ActivateMode>(LOWORD(wParam));
//                auto minState = HIWORD(wParam) != 0
//                    ? WindowSurface::MinState::Minimized
//                    : WindowSurface::MinState::NotMinimized;
//                surface->onActivate.notify(mode, minState);             
//            } break;
//
//            case WM_SETFOCUS:
//                surface->onGotFocus.notify();
//                break;
//
//            case WM_KILLFOCUS:
//                surface->onLostFocus.notify();
//                break;
//
//
//            case WM_KEYDOWN:
//            {
//                auto code = static_cast<uint8_t>(wParam);
//                auto bitfield = static_cast<WindowSurface::KeysBitfield>(static_cast<uint32_t>(lParam));
//                surface->onKeyDown.notify(code, bitfield);
//            } break;
//
//            case WM_KEYUP:
//            {
//                auto code = static_cast<uint8_t>(wParam);
//                auto bitfield = static_cast<WindowSurface::KeysBitfield>(static_cast<uint32_t>(lParam));
//                surface->onKeyUp.notify(code, bitfield);
//            } break;
//
//            case WM_CHAR:
//            {
//                auto unicode = static_cast<uint16_t>(wParam);
//                surface->onChar.notify(unicode);
//            } break;
//
//            case WM_LBUTTONDOWN:
//            case WM_LBUTTONUP:
//            case WM_RBUTTONDOWN:
//            case WM_RBUTTONUP:
//            case WM_MBUTTONDOWN:
//            case WM_MBUTTONUP:
//            case WM_XBUTTONDOWN:
//            case WM_XBUTTONUP:
//            //case WM_MOUSEMOVE:
//            {
//                //auto posX = static_cast<int16_t>(LOWORD(lParam));
//                //auto posY = static_cast<int16_t>(HIWORD(lParam));
//                auto btnState = static_cast<uint8_t>(LOWORD(wParam));
//                MouseState state{};
//                //state.relativePosition = { posX, posY };
//                state.buttonState.copyField(btnState);
//                surface->onMouseButton.notify(state);
//            } break;
//
//            case WM_MOUSEWHEEL:
//            {
//                //auto posX = //static_cast<int16_t>(LOWORD(lParam));
//                //auto posY = static_cast<int16_t>(HIWORD(lParam));
//                //auto btnState = static_cast<uint8_t>(LOWORD(wParam));
//                auto wheel = static_cast<int16_t>(HIWORD(wParam));
//                MouseState state{};
//                //state.relativePosition = { posX, posY };
//                //state.buttonState.copyField(btnState);
//                state.wheel = glm::i16vec2{ state.wheel.x, wheel };
//                surface->onMouseWheel.notify(state);
//            } break;
//
//            case WM_MOUSEHWHEEL:
//            {
//                //auto posX = static_cast<int16_t>(LOWORD(lParam));
//                //auto posY = static_cast<int16_t>(HIWORD(lParam));
//                //auto btnState = static_cast<uint8_t>(LOWORD(wParam));
//                auto wheel = static_cast<int16_t>(HIWORD(wParam));
//                MouseState state{};
//                //state.relativePosition = { posX, posY };
//                //state.buttonState.copyField(btnState);
//                state.wheel = glm::i16vec2{ wheel, state.wheel.y };
//                surface->onMouseWheel.notify(state);
//            } break;
//
//            case WM_QUIT:
//            {
//                surface->onQuit.notify();
//            } break;
//
//            case WM_DPICHANGED:
//            {
//                RECT* rp = reinterpret_cast<RECT*>(lParam);
//                int16_t dpi = static_cast<int16_t>(HIWORD(wParam));
//                IntRect<32> rect{ rp->left, rp->top, rp->right - rp->left, rp->bottom - rp->top };
//                surface->onDpiChanged.notify(dpi, rect);
//            } break;
//
//            case WM_INPUT:
//            {
//
//                RAWINPUT raw;
//                UINT size = sizeof(raw);
//                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
//
//                if (raw.header.dwType == RIM_TYPEMOUSE) {
//                    const RAWMOUSE& mouse = raw.data.mouse;
//                    if (mouse.usFlags == MOUSE_MOVE_RELATIVE) {
//                        int deltaX = mouse.lLastX;
//                        int deltaY = mouse.lLastY;
//
//                        POINT pos;
//                        int absX = 0, absY = 0;
//                        if (GetCursorPos(&pos)) {
//                            absX = pos.x;
//                            absY = pos.y;
//                        }
//
//                        surface->onMouseRaw.notify(MouseDataRaw{deltaX, deltaY, absX, absY});
//                    }
//                } else if (raw.header.dwType == RIM_TYPEKEYBOARD)
//                {
//                    const RAWKEYBOARD& keyboard = raw.data.keyboard;
//                    KeyEvent event = MapRawKeyboardEvent(keyboard);
//                    surface->onKeyEvent.notify(event);
//                }
//            } break;
//
//
//            default:
//                break;
//        }
//    }
//
//    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
//}
//
//static ErrorCode createSurface(
//    HINSTANCE hInstance,
//    HINSTANCE,
//    LPSTR,
//    int nCmdShow,
//    LPCWSTR className,
//    LPCWSTR windowTitle,
//    uint16_t width, uint16_t height,
//    WindowSurface* surface)
//{
//    WNDCLASSEXW wc = {};
//    wc.cbSize = sizeof(WNDCLASSEXW);
//    wc.lpfnWndProc = WindowProc;
//    wc.hInstance = hInstance;
//    wc.lpszClassName = className;
//
//    ATOM atom = RegisterClassExW(&wc);
//
//    if (atom == 0) {
//        DWORD err = GetLastError();
//        MessageBoxW(nullptr, L"RegisterClassW failed", L"Error", MB_OK);
//        return static_cast<ErrorCode>(err);
//    }
//
//    HWND g_hWnd = CreateWindowExW(
//        0,
//        className,
//        windowTitle,
//        WS_OVERLAPPEDWINDOW,
//        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
//        nullptr, nullptr, hInstance, surface
//    );
//
//    if (!g_hWnd) {
//        DWORD err = GetLastError();
//        return static_cast<ErrorCode>(err);
//    }
//
//    LONG style = GetWindowLong(g_hWnd, GWL_STYLE);
//    style &= ~WS_MAXIMIZEBOX;
//    style |= WS_MINIMIZEBOX;
//    SetWindowLong(g_hWnd, GWL_STYLE, style);
//
//    surface->windowHandle = g_hWnd;
//    surface->windowInstance = hInstance;
//    surface->width = width;
//    surface->height = height;
//
//    SetWindowTextW(g_hWnd, windowTitle);
//
//    return ErrorCode::OK;
//}

//INLINE static void HandleRawInputWin32(WindowSurface* const surface, HRAWINPUT hRawInput) {
//    UINT dataSize = 0;
//
//    // First, query required size
//    GetRawInputData(hRawInput, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
//
//    std::vector<BYTE> rawData(dataSize);
//
//    if (GetRawInputData(hRawInput, RID_INPUT, rawData.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize) {
//        // Handle error here
//        return;
//    }
//
//    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawData.data());
//
//    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
//        const RAWKEYBOARD& keyboard = raw->data.keyboard;
//
//        KeyEvent event = MapRawKeyboardEvent(keyboard);
//        surface->onKeyEvent.notify(event);
//    }
//}
//#endif // BUILD_WIN

#ifdef BUILD_GLFW

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>

class WindowContext {
    GLFWwindow* m_window{nullptr};
    std::string m_appName{};
    std::string m_windowTitle{};



public:
    uint16_t width{0};
    uint16_t height{0};
    std::string& appName() { return m_appName; }

    WindowContext() = default;
    WindowContext(WindowContext&& other) = default;
    WindowContext(const WindowContext&) = delete;
    WindowContext& operator=(const WindowContext&) = delete;

    template <std::integral I>
    static ErrorCode create(
        const char* appName, const char* wndTitle, const I width, const I height,
        WindowContext* outCtx)
    {
        if (outCtx == nullptr)
            return ErrorCode::GLFW_CONTEXT_IS_NULL;

        if (!glfwInit())
            return ErrorCode::GLFW_UNKNOWN_INIT_ERROR;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        outCtx->m_window = glfwCreateWindow(width, height, wndTitle, nullptr, nullptr);
        if (!outCtx->m_window )
            return ErrorCode::GLFW_FAILED_WND_CREATION;

        outCtx->m_appName = std::string{appName};
        outCtx->m_windowTitle = std::string{wndTitle};
        outCtx->width = static_cast<uint16_t>(width);
        outCtx->height = static_cast<uint16_t>(height);

        return ErrorCode::OK;
    }

    [[nodiscard]] const char* appName_c_str() const { return m_appName.c_str(); }

    GLFWwindow* window() const { return m_window; }
    GLFWwindow* window() { return m_window; }

    static void showWindow(int) {}

    static std::vector<const char*> getVulkanInstanceExtensions() {
        uint32_t count = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&count);
        return std::vector(exts, exts + count);
    }

    VkResult createSurface(const VkInstance instance, VkSurfaceKHR* out) const {
        return glfwCreateWindowSurface(instance, m_window, nullptr, out);
    }
};

#endif

#endif // WINDOWSURFACE_HPP