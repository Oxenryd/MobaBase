#ifndef WINDOWSURFACE_HPP
#define WINDOWSURFACE_HPP

#ifdef BUILD_WIN
#include <windows.h>
#endif

#include <cstdint>


struct WindowSurface
{
#ifdef BUILD_WIN
    HWND windowHandle;
    WNDCLASS windowClass;
    int nCmdShow;
    void showWindow() { ShowWindow(windowHandle, nCmdShow); }
    void closeWindow() { CloseWindow(windowHandle); }
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
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static WindowSurface createSurface(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow,
    const char* className,
    const char* windowTitle,
    uint16_t width, uint16_t height)
{
    HWND g_hWnd;
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    RegisterClass(&wc);

    g_hWnd = CreateWindowEx(
        0,
        className,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    WindowSurface ws;
    ws.nCmdShow = nCmdShow;
    ws.windowClass = wc;
    ws.windowHandle = g_hWnd;
    return ws;
}

#endif // BUILD_WIN

#endif // WINDOWSURFACE_HPP