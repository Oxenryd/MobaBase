// MobaBase.cpp : Defines the entry point for the application.
//

#include "Engine.h"

#ifdef IGPU_PRIO
	const bool igpuPriority = true;
#else
	const bool igpuPriority = false;
#endif

#ifdef BUILD_WIN


	#ifdef DEBUGGING
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>		

		void finalBreak() {
			Log::deInit();
			_CrtDumpMemoryLeaks();
			//__debugbreak();
		}
	#endif

constexpr const wchar_t* CLASS_NAME = L"VulkanTest";
constexpr const wchar_t* WINDOW_TITLE = L"VulkanTest";
constexpr const int WND_WIDTH = 1280;
constexpr const int WND_HEIGHT = 800;

size_t resizeTimes = 0;

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

void finalCleanup() {
	Log::deInit();
}

int __stdcall main(HINSTANCE hInstance, HINSTANCE instance, LPSTR str, int nCmdShow) {
	
#ifdef DEBUGGING
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	atexit(finalBreak);
#else
	atexit(finalCleanup);
#endif

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
	
	// Set up engine parameters
	InputManager* inputMan = mainArena.construct<InputManager>(wnd);
	Engine* engine = mainArena.construct<Engine>(mainArena);
	LOGLINE(LogType::Info, LogMod::Rendering, "Compiling Shaders...\n");
	ErrorCode EC = engine->getShaderManager()->recompileShaderCache();
	if (EC == ErrorCode::OK)
		LOG(LogType::Success, "\t\t\t\t\tDone. Compiled " + std::to_string(engine->getShaderManager()->totalShaders()) + " shaders.\n");
	else {
		LOG(LogType::Error, "Failed. Code: " + std::to_string(static_cast<uint8_t>(EC)));
		return (int)EC;
	}

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

	// Create Vulkan Context
	LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Vulkan context... ");
	VulkanContext* vkCtx = mainArena.construct<VulkanContext>(wnd);
	auto vk = vkCtx->initVulkan(
		VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR,
		engine->getShaderManager()->vertexShaders()[0], engine->getShaderManager()->pixelShaders()[0],
		igpuPriority);
	if (vk != VK_SUCCESS) {
		LOG(LogType::Error, "Failed. Code: " + std::to_string(static_cast<uint32_t>(vk)));
		return (int)vk;
	}
	LOGLINE(LogType::Success, LogMod::Vulkan, "Vulkan init Complete.\n");
	wnd->onResize.subscribe([vkCtx](WindowSurface::SizeType type, glm::u16vec2 newSize)
							{
								resizeTimes++;
								if (resizeTimes < 2) return;
								vkCtx->notifyViewResized(nullptr, newSize.x, newSize.y);
							});

	// Start Engine
	engine->setTargetUpdateDeltaTime(0.0);
	engine->start(vkCtx, inputMan);

	// Reset timing
	timeEndPeriod(1);
	return 0;
}

#endif // BUILD_WIN