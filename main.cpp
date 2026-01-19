// main.cpp : Defines the entry point for the application.

#ifdef BUILD_WIN
	#include "Engine.h"
	#include "Templates.hpp"
	#ifdef DEBUGGING
		#include "MemDebugFilter.hpp"
		#include <stdlib.h>	

		void finalBreak() {
			Log::deInit();
			//_CrtSetDumpClient(MyDumpClient);
			//DumpFilteredLeaksManual();
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
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
	//_CrtSetReportHook(MyReportHook);
	//_CrtSetBreakAlloc(14722);
	atexit(finalBreak);
#else
	atexit(finalCleanup);
#endif

	// Set windows tight thread sync
	timeBeginPeriod(1);

	// Setup logger
	Log::init<DefaultTerminalLogger>();

	// Create Engine
	Engine engine{ "VulkanTest", 256_MB };

	// Create Window surface
	LOGLINE(LogType::Info, LogMod::Window, "Creating Window Surface... ");
	ErrorCode EC = createSurface(
		hInstance, instance, nullptr, nCmdShow, CLASS_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT, engine.getWndSurface());
	if (EC_FAILED(EC)) {
		LOGLINE(LogType::Error, LogMod::Window, "Could not create HWND: " +
				std::to_string(static_cast<DWORD>(EC)));
		EC_RETURN_FAILED_INT(EC);
	}
	LOG(LogType::Success, "Done.");

	// Setup rest of engine
	EC = engine.init();
	EC_RETURN_FAILED_INT(EC); 

	// Callbacks for Window->Vulkan
	engine.getWndSurface()->onResize.subscribe([&engine](WindowSurface::SizeType type, glm::u16vec2 newSize)
							{
								resizeTimes++;
								if (resizeTimes < 2) return;
								bool pendingExit = 
									engine.getStatus() == EngineStatus::PendingStop ? true : false;
								engine.getVulkanContext()->notifyViewResized(&pendingExit, newSize.x, newSize.y);
							});
	engine.getWndSurface()->onClose.subscribe([&engine]()
						   {
							    engine.getVulkanContext()->setPendingExit();
								engine.stop();
						   });
	engine.onReadFPS.subscribe([&engine](Engine* engPtr, uint32_t frames)
							{
								static std::wstring windowTitle;
								windowTitle = L"VulkanTest - " + std::to_wstring(frames) + L" FPS\t" + 
									std::to_wstring(RenderManager::getInstance()->vkContext()->lastDrawcallCount()) + L" drawcalls\t" +
									std::to_wstring(RenderManager::getInstance()->vkContext()->lastPipelineSwitchCount()) + L" pipeline binds\t";
								SetWindowTextW(engine.getWndSurface()->windowHandle, windowTitle.c_str());
							});

	// Start Engine
	engine.createNewScene<GameScene>(512_MB, nullptr);
	engine.setTargetUpdateDeltaTime(0.0);
	engine.start();

	// Reset timing
	timeEndPeriod(1);
	return 0;
}

#else

#include "Engine.h"
#include "WindowContext.h"
#include "Templates.hpp"


auto APP_NAME = "VulkanTest";
auto WINDOW_TITLE = "VulkanTest";
constexpr unsigned short WND_WIDTH = 1280;
constexpr unsigned short WND_HEIGHT = 800;

int main(int, char*)
{
	ErrorCode EC{};

	// Setup logger
	Log::init<DefaultTerminalLogger>();

	// Create Engine
	Engine engine{ APP_NAME, 256_MB };

	// Create Window Context
	WindowContext wndCtx{};
	EC = WindowContext::create(APP_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT, &wndCtx);
	if (EC_FAILED(EC)) {
		LOGLINE(LogType::Error, LogMod::Window, "Could not create Window Surface... ");
		return static_cast<int>(EC);
	}
	// Setup rest of engine
	EC = engine.init(&wndCtx);
	EC_RETURN_FAILED_INT(EC);

	// Start Engine
	// engine.createNewScene<GameScene>(512_MB, nullptr);
	// engine.setTargetUpdateDeltaTime(0.0);
	// engine.start();

	return 0;
}


#endif // BUILD_WIN

