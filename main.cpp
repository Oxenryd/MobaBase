// MobaBase.cpp : Defines the entry point for the application.
//

#include "Engine.h"

constexpr const wchar_t* CLASS_NAME = L"VulkanTest";
constexpr const wchar_t* WINDOW_TITLE = L"VulkanTest";
constexpr const int WND_WIDTH = 1280;
constexpr const int WND_HEIGHT = 800;

#ifdef BUILD_WIN

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

int __stdcall main(HINSTANCE hInstance, HINSTANCE instance, LPSTR str, int nCmdShow) {
	
	// Set windows tight thread sync
	timeBeginPeriod(1);

	// Setup logger
	Log::init<DefaultTerminalLogger>();

	// Setup base HeapArena
	HeapArena mainArena{ 2048 * 2048 };

	// Create Window Handle
	WindowSurface* wnd = mainArena.construct<WindowSurface>();
	wnd->enableRawInput();
	wnd->appName = "VulkanTest";
	auto wndResult = createSurface(
		hInstance, instance, nullptr, nCmdShow, CLASS_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT, wnd);
	if (wndResult != ErrorCode::OK) {
		LOGLINE(LogType::Error, LogMod::Window, "Could not create HWND: " + 
				std::to_string(static_cast<DWORD>(wndResult)));
		return (int)wndResult;
	}

	// Create Vulkan Context
	VkResult vkResult{};
	VulkanContext* vkCtx = mainArena.construct<VulkanContext>(wnd);
	Vk_CHECK(vkResult, vkCtx->create(wnd));
	Vk_CHECK(vkResult, vkCtx->createSwapchain(wnd, VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR));
	
	// Set up engine parameters
	InputManager* inputMan = mainArena.construct<InputManager>(wnd);
	Engine* engine = mainArena.construct<Engine>(mainArena);
	LOGLINE(LogType::Info, LogMod::Rendering, "Compiling Shaders...\n");
	ErrorCode EC = engine->getShaderManager()->recompileShaderCache();
	if (EC == ErrorCode::OK)
		LOG(LogType::Success, "Done. Compiled " + std::to_string(engine->getShaderManager()->totalShaders()) + " shaders.");
	else
		LOG(LogType::Error, "Failed. Code: " + std::to_string(static_cast<uint8_t>(EC)));

	wnd->onClose.subscribe([engine]()
						   {
							   engine->stop();
						   });
	engine->onReadFPS.subscribe([&wnd](Engine* engPtr, uint32_t frames)
		{
			static std::wstring windowTitle;
			windowTitle = L"VulkanTest - " + std::to_wstring(frames) + L" FPS";
			SetWindowTextW(wnd->windowHandle, windowTitle.c_str());
		});
	engine->start(wnd, inputMan);	

	// Reset timing
	timeEndPeriod(1);
	return 0;
}

#endif // BUILD_WIN