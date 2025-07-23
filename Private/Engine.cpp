#include "Engine.h"
#include "RenderManager.h"
#include <immintrin.h>

Engine::Engine(HeapArena& heap) :
	m_targetUpdateDeltaTime{ FPS_400 },
	m_updateDeltaTime{ 0.0 },
	m_lastUpdateTime{ std::chrono::steady_clock::now() },
	m_options{},
	m_targetFixedDeltaTime{ FPS_60 },
	m_fixedAccu{ 0.0 },
	m_framesSinceLastFpsRead{ 0 },
	m_baseTimers{ 1 },
	m_lastReadFps{m_targetUpdateDeltaTime},
	m_fpsCountTimer{ m_baseTimers.createTimer(true, 1.0) },
	m_baseArena{heap}
 {
	assert(s_instance == nullptr && "THERE CAN BE ONLY ONE!");

	s_instance = this;
	m_baseTimers.m_incOnTimes[m_fpsCountTimer.m_timerIndex].subscribe([this]()
		{
			onReadFPS.notify(this, m_framesSinceLastFpsRead);
			m_framesSinceLastFpsRead = 0;
		});

	_initShaderManager();
}

Engine::~Engine() {
	for (auto* scene : m_scenes) {
		delete scene;
	}
}


inline double Engine::_tickDt() {
	using namespace std::chrono;

	for (;;) {
		auto now = steady_clock::now();
		double remaining = m_targetUpdateDeltaTime - duration<double>(now - m_lastUpdateTime).count();
		if (remaining <= 0.0)
			break;

		if (remaining > m_options.deltaTimeJitterThreshold)
			std::this_thread::sleep_for(0.001s);
		else {
			_mm_pause();
		}
	}


	auto frameStart = steady_clock::now();
	double frameTime = std::min(
		duration<double>(frameStart - m_lastUpdateTime).count(),
		MAX_DELTA_TIME);

	m_updateDeltaTime = frameTime;
	m_lastUpdateTime = frameStart;
	return m_updateDeltaTime;
}

void Engine::_initShaderManager() {
	LOGLINE_IND(LogType::Info, LogMod::Rendering, "Setting up ShaderManager... ", 1);
#ifdef BUILD_WIN

	DxcWin32VulkanShaderCompiler* w32VkCompiler = m_baseArena.construct<DxcWin32VulkanShaderCompiler>();
	m_renderMan = m_baseArena.construct<RenderManager>(RenderManager{ w32VkCompiler });

#endif

#ifdef SHADER_HOTRELOAD
		LOGLINE(LogType::Info, LogMod::Rendering, "Shader HotReload is ON.");
		m_renderMan->m_hotreloadTimer = new Timer{ m_baseTimers.createTimer(true, 1.5) };
		m_baseTimers.m_incOnTimes[m_renderMan->m_hotreloadTimer->m_timerIndex].subscribe([this]()
								{
									m_renderMan->hotReload();
								});
#else
		LOGLINE(LogType::Info, LogMod::Rendering, "Shader HotReload is OFF.");
#endif
		RenderManager::s_instance = m_renderMan;
		LOGLINE_IND(LogType::Success, LogMod::Rendering, "ShaderManager initialized.", -1);
}

void Engine::start(VulkanContext* graphicContext, InputManager* inputPtr) {

	if (m_status != EngineStatus::Stopped) {
		LOGLINE(LogType::Warning, LogMod::Engine, "Tried to start while not stopped. Ignoring.");
		return;
	}

	LOGLINE_IND(LogType::Info, LogMod::Engine, "Starting... ", 1);

	// No scenes, create default and run.
	if (m_scenes.empty()) {
		LOGLINE(LogType::Remark, LogMod::Engine, "Creating a default scene... ");
		auto* scenePtr = this->createNewScene<DefaultScene>(nullptr);
		m_activeSceneIndices.insert(0);
		LOGLINE(LogType::Remark, LogMod::Engine, "Scene created.");
	} else if (m_activeSceneIndices.empty())
		m_activeSceneIndices.insert(0);

	// Set main window
	m_options.inputManager = inputPtr;
	m_options.graphicContext = graphicContext;

	m_status = EngineStatus::PendingRun;
	onStartInitiated.notify(this);
	
	m_options.graphicContext->windowSurface->showWindow(SW_NORMAL);

	m_status = EngineStatus::Running;
	onStarted.notify(this);

	m_lastUpdateTime = std::chrono::steady_clock::now();

	LOGLINE_IND(LogType::Success, LogMod::Engine, "Engine running! ", -1);
	_run();

}


inline void Engine::_run() {

	m_baseTimers.startTimer(m_fpsCountTimer);

	while (m_status != EngineStatus::PendingStop) {
		auto dt = _tickDt();
		m_fixedAccu += dt;

		while (m_fixedAccu >= m_targetFixedDeltaTime) {
			_updateFixed();
			m_fixedAccu -= m_targetFixedDeltaTime;
		}

		//// Scene transit
		//if (m_sceneTransitionRequested) {
		//	switch (m_sceneTransitMode)
		//	{
		//		case SceneTransitionMode::DontRunTransitioning:
		//		{
		//			m_scenes[m_requestedSceneIndex]->loadDispatch();
		//			m_scenes[m_curSceneIndex]->unloadDispatch();
		//			m_curSceneIndex = m_requestedSceneIndex;
		//			m_sceneTransitionRequested = false;
		//		} break;

		//		case SceneTransitionMode::RunOnce:
		//		{
		//			m_scenes[m_curSceneIndex]->transitioningDispatch();
		//			m_scenes[m_requestedSceneIndex]->loadDispatch();
		//			m_scenes[m_curSceneIndex]->unloadDispatch();
		//			m_curSceneIndex = m_requestedSceneIndex;
		//			m_sceneTransitionRequested = false;
		//		} break;

		//		case SceneTransitionMode::WaitForDone:
		//		{
		//			auto doneStatus = m_scenes[m_curSceneIndex]->transitioningDispatch();
		//			if (doneStatus == SceneTransitionStatus::Done) {
		//				m_scenes[m_requestedSceneIndex]->loadDispatch();
		//				m_scenes[m_curSceneIndex]->unloadDispatch();
		//				m_curSceneIndex = m_requestedSceneIndex;
		//				m_sceneTransitionRequested = false;
		//			}
		//		} break;
		//	}
		//}

		m_options.inputManager->pollEvents();
		_updateEarly(dt);
		_updateLate(dt);

		m_baseTimers.update(dt);

		


		VulkanContext::RenderContext drawCtx{};	
		drawCtx.frameCount = m_totalFrames;


		m_options.graphicContext->draw(static_cast<void*>(&drawCtx));
		m_framesSinceLastFpsRead++;
		m_totalFrames++;
		std::this_thread::yield();
	}

	m_options.graphicContext->windowSurface->destroyWindow();

	m_status = EngineStatus::Stopped;
	onStopped.notify(this);
	LOG(LogType::Success, "Done.");
}

void Engine::stop() {
	LOGLINE(LogType::Info, LogMod::Engine, "Stopping... ");
	m_status = EngineStatus::PendingStop;
	onPendingStop.notify(this);
}

inline void Engine::_updateEarly(double dt) {
	onEarlyUpdateEnter.notify(this);
	for (auto& index : m_activeSceneIndices) {
		if (m_scenes[index]->isFirstFrame())
			m_scenes[index]->startDispatch();

		m_scenes[index]->updateDispatch(dt);	
	}
	onEarlyUpdateExit.notify(this);
}

inline void Engine::_updateLate(double dt) {
	onLateUpdateEnter.notify(this);
	for (auto& index : m_activeSceneIndices) 
		m_scenes[index]->lateUpdateDispatch(dt);
	
	onLateUpdateExit.notify(this);
}

inline void Engine::_updateFixed() {
	onFixedUpdateEnter.notify(this);
	for (auto& index : m_activeSceneIndices)
		m_scenes[index]->fixedUpdateDispatch();

	onFixedUpdateExit.notify(this);
}
