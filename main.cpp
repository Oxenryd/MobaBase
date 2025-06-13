// MobaBase.cpp : Defines the entry point for the application.
//

#include "Engine.h"

constexpr const wchar_t* CLASS_NAME = L"TestProgram";
constexpr const wchar_t* WINDOW_TITLE = L"Window";
constexpr const int WND_WIDTH = 1280;
constexpr const int WND_HEIGHT = 800;

#ifdef BUILD_WIN

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

int __stdcall main(HINSTANCE hInstance, HINSTANCE instance, LPSTR str, int nCmdShow) {
	
	// Set windows tight thread sync
	timeBeginPeriod(1);

	// Create Window Handle
	WindowSurface wnd{};
	auto wndResult = createSurface(
		hInstance, instance, nullptr, nCmdShow, CLASS_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT, &wnd);
	if (wndResult != ErrorCode::OK) {
		LOGLINE(LogType::Error, "Could not create HWND: " + std::to_string(static_cast<DWORD>(wndResult)));
		return (int)wndResult;
	}

	// Create Vulkan Context
	VkResult vkResult{};
	VulkanContext vkCtx{};
	Vk_CHECK(vkResult, vkCtx.create(&wnd));
	Vk_CHECK(vkResult, vkCtx.CreateSwapchain(&wnd));
	


	// Set up engine parameters
	InputManager inputMan{ &wnd };
	Engine engine{};
	Log::init<DefaultTerminalLogger>();
	engine.onEarlyUpdateEnter.subscribe([&wnd](Engine* engPtr)
		{
			static std::wstring windowTitle;
			windowTitle = L"TestProgram - " + std::to_wstring(1.0 / engPtr->deltaTime()) + L" FPS";
			SetWindowTextW(wnd.windowHandle, windowTitle.c_str());
		});
	engine.start(&wnd, &inputMan);	

	// Reset timing
	timeEndPeriod(1);
	return 0;
}

#endif // BUILD_WIN