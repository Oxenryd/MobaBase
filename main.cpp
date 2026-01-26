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

auto APP_NAME = "VulkanTest";
auto WINDOW_TITLE = "VulkanTest";
constexpr unsigned short WND_WIDTH = 1280;
constexpr unsigned short WND_HEIGHT = 800;

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
	Engine engine{ APP_NAME, 256_MB };

	// Create Window Context
	WindowContext wndCtx{};
	ErrorCode EC = WindowContext::create(APP_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT, &wndCtx);
	if (EC_FAILED(EC)) {
		LOGLINE(LogType::Error, LogMod::Window, "Could not create Window Surface... ");
		return static_cast<int>(EC);
	}
	// Setup rest of engine
	EC = engine.init(&wndCtx);
	EC_RETURN_FAILED_INT(EC);

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
	// Setup logger
	Log::init<DefaultTerminalLogger>();

	// Create Engine
	Engine engine{ APP_NAME, 256_MB };

	// Create Window Context
	WindowContext wndCtx{};
	ErrorCode EC = WindowContext::create(APP_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT, &wndCtx);
	if (EC_FAILED(EC)) {
		LOGLINE(LogType::Error, LogMod::Window, "Could not create Window Surface... ");
		return static_cast<int>(EC);
	}
	// Setup rest of engine
	EC = engine.init(&wndCtx);
	EC_RETURN_FAILED_INT(EC);

	// Start Engine
	engine.createNewScene<GameScene>(512_MB, nullptr);
	engine.setTargetUpdateDeltaTime(0.0);
	engine.start();

	return 0;
}


#endif // BUILD_WIN

