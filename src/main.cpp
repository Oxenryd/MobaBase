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


size_t resizeTimes = 0;

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

void finalCleanup() {
	Log::deInit();
}
#endif // BUILD_WIN

#include "Engine.h"
#include "WindowContext.h"
#include "VulkanContext.hpp"
#include "Templates.hpp"

auto APP_NAME = "VulkanTest";
auto WINDOW_TITLE = "VulkanTest";
constexpr unsigned short WND_WIDTH = 1280;
constexpr unsigned short WND_HEIGHT = 800;

int main(int, char**)
{
	// Setup logger
	Log::init<DefaultTerminalLogger>();

	// Create Window Context
	auto wndCtx = std::make_unique<WindowContext>(APP_NAME, WINDOW_TITLE, WND_WIDTH, WND_HEIGHT);
	if (EC_FAILED(wndCtx->getCurrentError())) {
		LOGLINE(LogType::Error, LogMod::Window, "Could not create Window Surface... ");
		return static_cast<int>(wndCtx->getCurrentError());
	}

	// Create engine
	Engine engine{ std::move(wndCtx), APP_NAME };
	EC_RETURN_FAILED_INT(engine.getCurrentError());

	// Print information on title
	engine.onReadFPS.subscribe( [&](const Engine* eng, const uint32_t fps)
		{
			static std::string str{};
			const uint32_t dCalls = eng->getVulkanContext()->lastDrawcallCount();
			str = std::format("{} - FPS: {} - Draw Calls: {}", APP_NAME, fps, dCalls);
			eng->getWindowContext()->setWindowTitle(str);
		});

	// Start Engine
	engine.createScene<GameScene>(512_MB, nullptr);
	engine.setTargetUpdateDeltaTime(0.0);
	engine.start();

	return 0;
}




