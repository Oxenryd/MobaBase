#ifndef WINDOWSURFACE_HPP
#define WINDOWSURFACE_HPP

#ifdef BUILD_WIN
    #ifndef _INC_WINAPIFAMILY
        #include <windows.h>
    #endif
#endif

#include <cstdint>

#include "ErrorCodes.hpp"

struct WindowSurface
{
    uint16_t width;
    uint16_t height;
    std::string appName;

#ifdef BUILD_WIN
    WindowSurface() = default;
    WindowSurface(const WindowSurface& other) : 
        windowHandle{ other.windowHandle }
    {}
    WindowSurface& operator=(const WindowSurface& rhs) {
        windowHandle = rhs.windowHandle;
        return *this;
    }

    HWND windowHandle;
    HINSTANCE windowInstance;
    void showWindow(int showCommand) { 
        ShowWindow(windowHandle, showCommand);
    }
    void destroyWindow() { DestroyWindow(windowHandle); }

#endif // BUILD_WIN
};


#ifdef BUILD_WIN

// Basic window procedure
static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
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
        nullptr, nullptr, hInstance, nullptr
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
    return ErrorCode::OK;
}

#endif // BUILD_WIN

#endif // WINDOWSURFACE_HPP