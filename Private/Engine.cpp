#include "Engine.h"
#include "RenderManager.h"
#include <immintrin.h>

thread_local std::unordered_map<uint16_t, size_t> SceneRenderSystem::s_threadIndex;


Engine::Engine(const std::string& appName, size_t baseSize) :
	m_appName{appName},
	m_targetUpdateDeltaTime{ FPS_400 },
	m_updateDeltaTime{ 0.0 },
	m_lastUpdateTime{ std::chrono::steady_clock::now() },
	m_targetFixedDeltaTime{ FPS_60 },
	m_fixedAccu{ 0.0 },
	m_framesSinceLastFpsRead{ 0 },
	m_baseTimers{ 1 },
	m_lastReadFps{m_targetUpdateDeltaTime},
	m_fpsCountTimer{ m_baseTimers.createTimer(true, 1.0) },
	m_baseArena{baseSize}
 {
	assert(s_instance == nullptr && "THERE CAN BE ONLY ONE!");

	s_instance = this;

	m_wnd = m_baseArena.construct<WindowSurface>();
	m_wnd->enableRawInput();

	m_baseTimers.m_incOnTimes[m_fpsCountTimer.m_timerIndex].subscribe([this]()
		{
			onReadFPS.notify(this, m_framesSinceLastFpsRead);
			m_framesSinceLastFpsRead = 0;
		});
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

		if (remaining > DEFAULT_DELTATIME_JITTER_SETTING)
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

ErrorCode Engine::_initShaderManager() {
	LOGLINE_IND(LogType::Info, LogMod::Rendering, "Setting up ShaderManager... ", 1);
#ifdef BUILD_WIN

	DxcWin32VulkanShaderCompiler* w32VkCompiler = m_baseArena.construct<DxcWin32VulkanShaderCompiler>();
	m_renderMan = m_baseArena.construct<RenderManager>(RenderManager{m_vkCtx, w32VkCompiler, 128_MB });

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
		return ErrorCode::OK;
}

ErrorCode Engine::_initInputManager() {
	m_inputMan = baseArena().construct<InputManager>(m_wnd);

	return ErrorCode::OK;
}

ErrorCode Engine::_initGraphics() {

	// Create Vulkan Context
	LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Vulkan context... ");
	m_vkCtx = m_baseArena.construct<VulkanContext>(m_wnd);
	VkPresentModeKHR presentMode;
#ifdef IGPU_PRIO
	presentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
#else
	presentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
#endif

#ifdef IGPU_PRIO
	const bool igpuPriority = true;
#else
	const bool igpuPriority = false;
#endif

	auto vk = m_vkCtx->initVulkan(presentMode, igpuPriority);
	if (vk != VK_SUCCESS) {
		LOG(LogType::Error, "Failed. Code: " + std::to_string(static_cast<uint32_t>(vk)));
		return (ErrorCode)vk;
	}
	LOGLINE(LogType::Success, LogMod::Vulkan, "Vulkan init Complete.\n");

	return ErrorCode::OK;
}

ErrorCode Engine::_initBaseShaders() {

	LOGLINE(LogType::Info, LogMod::Rendering, "Compiling Shaders...\n");
	ErrorCode EC = m_renderMan->recompileShaderCache();
	if (EC == ErrorCode::OK)
		LOG(LogType::Success,
			"\t\t\t\t\tDone. Compiled " + std::to_string(m_renderMan->totalShaders()) + " shaders.\n");
	else {
		LOG(LogType::Error, "Failed. Code: " + std::to_string(static_cast<uint8_t>(EC)));
		return (ErrorCode)EC;
	}

	// Create Base Materials
	auto* baseVs = getRenderManager()->getShader(MAT_BASE_VS);
	auto* basePs = getRenderManager()->getShader(MAT_BASE_PS);
	auto& baseMat = Material::createMaterial("BaseMaterialUnlit", *baseVs, *basePs);//Material{ "BaseMaterialUnlit", *baseVs, *basePs };
	baseMat.createInstance();
	m_vkCtx->createPipelineFromMaterial(m_renderMan, baseMat);
	std::cout << "\n";
	baseMat.debugPrintMaterialInfo();

	//auto* spriteVs = getRenderManager()->getShader("SpriteBatchVS");
	//auto* spritePs = getRenderManager()->getShader("SpriteBatchPS");
	//auto& spriteMat = Material::createMaterial( "SpriteMaterialUnlit", *spriteVs, *spritePs );
	//spriteMat.createInstance();
	//m_vkCtx->createPipelineFromMaterial(m_renderMan, spriteMat);
	//std::cout << "\n";
	//spriteMat.debugPrintMaterialInfo();


	// DEBUG! ////////////////////////////////////////////////////////////////////////////
	//auto& matInstance = spriteMat.createInstance();
	//glm::vec2 size = { 12.2f, 13.5f };
	//matInstance.setParameter("spriteInstances", "size", &size);
	//auto checkedSize = matInstance.getParameter<glm::vec2>("spriteInstances", "size");


	std::cout << "\n";
	//////////////////////////////////////////////////////////////////////////////////////
	

	

	return ErrorCode::OK;
}

ErrorCode Engine::_initBaseCallbacks() {

	// Shader Hotreloaded
#ifdef SHADER_HOTRELOAD	
	m_renderMan->onShaderHotReloaded.subscribe([this](void*) {
		this->getVulkanContext()->resetPipeline(0,
							 this->m_renderMan->vertexShaders()[0],
							 this->m_renderMan->pixelShaders()[0]);
															  });
#endif
	return ErrorCode::OK;
}


void Engine::start() {

	if (m_status != EngineStatus::Stopped) {
		LOGLINE(LogType::Warning, LogMod::Engine, "Tried to start while not stopped. Ignoring.");
		return;
	}

	LOGLINE_IND(LogType::Info, LogMod::Engine, "Starting... ", 1);

	// No scenes, create default and run.
	if (m_scenes.empty()) {
		LOGLINE(LogType::Remark, LogMod::Engine, "Creating a default scene... ");
		auto* scenePtr = this->createNewScene<DefaultScene>(512_MB, nullptr);
		m_activeSceneIndices.insert(0);
		LOGLINE(LogType::Remark, LogMod::Engine, "Scene created.");
	} else if (m_activeSceneIndices.empty())
		m_activeSceneIndices.insert(0);


	m_status = EngineStatus::PendingRun;
	onStartInitiated.notify(this);
	
	m_wnd->showWindow(SW_NORMAL);

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

		m_inputMan->pollEvents();
		_updateEarly(dt);
		_updateLate(dt);

		m_baseTimers.update(dt);

		


		VulkanContext::RenderContext drawCtx{};	
		drawCtx.frameCount = m_totalFrames;


		m_vkCtx->draw(static_cast<void*>(&drawCtx));
		m_framesSinceLastFpsRead++;
		m_totalFrames++;
		std::this_thread::yield();
	}

	for (auto scene : m_scenes) {
		scene->unloadDispatch();
	}
	m_scenes.clear();

	m_wnd->destroyWindow();

	m_status = EngineStatus::Stopped;
	onStopped.notify(this);
	LOG(LogType::Success, "Done.");
}

void Engine::stop() {
	LOGLINE(LogType::Info, LogMod::Engine, "Stopping... ");
	m_status = EngineStatus::PendingStop;
	onPendingStop.notify(this);
}

ErrorCode Engine::init() {

	ErrorCode EC{};
	EC_CHECK(EC, _initGraphics());

	EC_CHECK(EC, _initShaderManager());
	EC_CHECK(EC, _initInputManager());
	
	EC_CHECK(EC, _initBaseShaders());
	EC_CHECK(EC, _initBaseCallbacks());

	return ErrorCode::OK;
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
	std::vector<uint32_t> removeIndices;
	for (auto& index : m_activeSceneIndices) {
		m_scenes[index]->lateUpdateDispatch(dt);
		if (m_scenes[index]->pendingUnload()) {
			m_scenes[index]->unloadDispatch();
			delete m_scenes[index];
			auto it = m_scenes.begin() + index;
			m_scenes.erase(it);
			removeIndices.push_back(index);
		}
	}
	for (auto& index : removeIndices) {
		m_activeSceneIndices.erase(index);
	}
		
	onLateUpdateExit.notify(this);
}

inline void Engine::_updateFixed() {
	onFixedUpdateEnter.notify(this);
	for (auto& index : m_activeSceneIndices)
		m_scenes[index]->fixedUpdateDispatch();

	onFixedUpdateExit.notify(this);
}
