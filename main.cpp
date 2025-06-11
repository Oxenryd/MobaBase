// MobaBase.cpp : Defines the entry point for the application.
//

#include "Engine.h"

constexpr const char* CLASS_NAME = "TestProgram";
constexpr const char* WINDOW_TITLE = "Window";
constexpr const int WND_WIDTH = 1280;
constexpr const int WND_HEIGHT = 800;

#ifdef BUILD_WIN

int __stdcall main(HINSTANCE hInstance, HINSTANCE instance, LPSTR str, int nCmdShow) {

	WindowSurface wnd = createSurface(
		hInstance, instance, nullptr, nCmdShow, CLASS_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT);
	InputManager inputMan{ &wnd };
	Engine engine{};
	wnd.showWindow();

	Debug::sleepBlock(2500);

	wnd.closeWindow();
	return 0;
}

#endif // BUILD_WIN