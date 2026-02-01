#include "Engine.h"
#include "RenderManager.h"
#include "GameObjectSystem.hpp"
#include "Profiler.hpp"

#include <immintrin.h>

thread_local std::unordered_map<uint16_t, size_t> SceneRenderSystem::s_threadIndex;


Engine::Engine(const char* appName, const size_t heapSize) :
	m_baseArena{heapSize},
	m_appName{appName},
	m_targetUpdateDeltaTime{ FPS_400 },
	m_targetFixedDeltaTime{ FPS_60 },
	m_lastReadFps{m_targetUpdateDeltaTime},
	m_lastUpdateTime{ std::chrono::steady_clock::now() },
	m_baseTimers{ 1 },
	m_fpsCountTimer{ m_baseTimers.createTimer(true, 1.0) }
 {
	assert(s_instance == nullptr && "THERE CAN BE ONLY ONE!");

	s_instance = this;

	//m_wnd = m_baseArena.construct<WindowContext>();
	

	m_baseTimers.m_incOnTimes[m_fpsCountTimer.m_timerIndex].subscribe([this]()
		{
			onReadFPS.notify(this, m_framesSinceLastFpsRead);
			m_framesSinceLastFpsRead = 0;
		});
}

Engine::~Engine() {

	if (m_workArena) {
		MWork::shutdown();
		delete m_workArena;
		m_workArena = nullptr;
	}

	for (const auto* scene : m_scenes) {
		delete scene;
	}
}


inline double Engine::_tickDt() {
	using namespace std::chrono;

	for (;;) {
		auto now = steady_clock::now();
		const double remaining = m_targetUpdateDeltaTime - duration<double>(now - m_lastUpdateTime).count();
		if (remaining <= 0.0)
			break;

		if (remaining > DEFAULT_DELTATIME_JITTER_SETTING)
			std::this_thread::sleep_for(0.001s);
		else {
			_mm_pause();
		}
	}


	const auto frameStart = steady_clock::now();
	const double frameTime = std::min(
		duration<double>(frameStart - m_lastUpdateTime).count(),
		MAX_DELTA_TIME);

	m_updateDeltaTime = frameTime;
	m_lastUpdateTime = frameStart;
	return m_updateDeltaTime;
}

ErrorCode Engine::_initShaderManager() {
	LOGLINE_IND(LogType::Info, LogMod::Rendering, "Setting up ShaderManager... ", 1);

	m_renderMan = m_baseArena.construct<RenderManager>(RenderManager{m_vkCtx, 128_MB });



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
	m_inputMan = baseArena().construct<InputManager>(m_wnd); //new InputManager(m_wnd);

	return ErrorCode::OK;
}

ErrorCode Engine::_initGraphics() {

	// Create Vulkan Context
	LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Vulkan context... ");
	m_vkCtx = m_baseArena.construct<VulkanContext>(); //new VulkanContext();
#ifdef IGPU_PRIO
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
	//presentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
#endif

#ifdef IGPU_PRIO
	constexpr bool igpuPriority = true;
#else
	constexpr bool igpuPriority = false;
#endif

#ifdef BUILD_GLFW

	auto vk = m_vkCtx->initVulkan(m_wnd, presentMode, igpuPriority);
#else
	auto vk = m_vkCtx->initVulkan(presentMode, nullptr, igpuPriority);
#endif
	if (vk != VK_SUCCESS) {
		LOG(LogType::Error, "Failed. Code: " + std::to_string(static_cast<uint32_t>(vk)));
		return static_cast<ErrorCode>(vk);
	}
	LOGLINE(LogType::Success, LogMod::Vulkan, "Vulkan init Complete.\n");

	return ErrorCode::OK;
}

ErrorCode Engine::_initBaseShaders() const {

	LOGLINE(LogType::Info, LogMod::Rendering, "Compiling Shaders...\n");
	ErrorCode EC = m_renderMan->recompileShaderCache();
	if (EC == ErrorCode::OK)
		LOG(LogType::Success,
			"\t\t\t\t\tDone. Compiled " + std::to_string(m_renderMan->totalShaders()) + " shaders.\n");
	else {
		LOG(LogType::Error, "Failed. Code: " + std::to_string(static_cast<uint8_t>(EC)));
		return EC;
	}

	// Create Base Materials          
	auto* baseVs = getRenderManager()->getShader(SHADER_BASE_VS);
	auto* basePs = getRenderManager()->getShader(SHADER_BASE_PS);
	auto& baseMat = Material::createMaterial("BaseMaterialUnlit", *baseVs, *basePs);
	baseMat.blendModes[0] = BlendMode::Alpha;
	baseMat.createInstance();
	m_vkCtx->createPipelineFromMaterial(m_renderMan, baseMat);
	std::cout << "\n";
	baseMat.debugPrintMaterialInfo();


	auto* shapeVS = getRenderManager()->getShader(SHADER_SHAPERENDERER_VS);
	auto* shapePS = getRenderManager()->getShader(SHADER_SHAPERENDERER_PS);
	auto& shapeMat = Material::createMaterial("ShapeRendererMaterial", *shapeVS, *shapePS);
	shapeMat.blendModes[0] = BlendMode::Alpha;
	shapeMat.createInstance();
	m_vkCtx->createPipelineFromMaterial(m_renderMan, shapeMat);
	std::cout << "\n";
	shapeMat.debugPrintMaterialInfo();

	std::cout << "\n";
	
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

ErrorCode Engine::_initJobSystem() {

	m_workArena = new FrameArena{ 32_MB };
	MWork::init(std::thread::hardware_concurrency(), WORK_QUEUE_CAP, m_workArena);


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
		[[maybe_unused]] auto* scenePtr = this->createNewScene<DefaultScene>(512_MB, nullptr);
		m_activeSceneIndices.insert(0);
		LOGLINE(LogType::Remark, LogMod::Engine, "Scene created.");
	} else if (m_activeSceneIndices.empty())
		m_activeSceneIndices.insert(0);


	m_status = EngineStatus::PendingRun;
	onStartInitiated.notify(this);
	
	m_wnd->showWindow(0);

	m_status = EngineStatus::Running;
	onStarted.notify(this);

	m_lastUpdateTime = std::chrono::steady_clock::now();

	LOGLINE_IND(LogType::Success, LogMod::Engine, "Engine running! ", -1);
	_run();

}


inline void Engine::_run() {

	m_baseTimers.startTimer(m_fpsCountTimer);

	while (m_status != EngineStatus::PendingStop && !glfwWindowShouldClose(m_vkCtx->window())) {

#ifdef PROFILER
		if (m_pendingTrace) {
			m_framesTraced = 0;
			LOGLINE(LogType::Info, LogMod::Engine, "Starting Profiler... ");
			PROFILE_BEGIN_SESSION(ENGINE_NAME, PROFILE_PATH);
			m_pendingTrace = false;
		}	

		if (m_framesTraced != UINT32_INVALID) {
			m_framesTraced++;
			if (m_framesTraced > m_framesToTrace) {
				PROFILE_END_SESSION();
				LOGLINE(LogType::Success, LogMod::Engine, "Trace Complete. ");
				m_framesTraced = UINT32_INVALID;
			}
		}
#endif

		PROFILE_SCOPE("Frame");
		PROFILE_FRAME_MARK(m_totalFrames);

		glfwPollEvents();

		const auto dt = _tickDt();
		m_updateDeltaTime = dt;
		m_fixedAccu += dt;
		m_totalTime += dt;
		m_baseCosD = std::cos(m_totalTime);
		m_baseSinD = std::sin(m_totalTime);
		m_baseCosF = static_cast<float>(m_baseCosD);
		m_baseCosF = static_cast<float>(m_baseSinD);
		{
			PROFILE_SCOPE("FixedDelta");
			while (m_fixedAccu >= m_targetFixedDeltaTime) {
				_updateFixed();
				m_fixedAccu -= m_targetFixedDeltaTime;
			}
		}

		//Wait for last frames jobs	before resetting arena
		{
			PROFILE_SCOPE("MWork_WaitPoint_NextFrame");
			MWork::waitAwaitPoint(MWork::JobAwaitPoint::StartNextFrame);
			MWork::reset();
		}

		{
			PROFILE_SCOPE("Input");
			m_inputMan->update(dt);
		}


		_updateEarly(dt);


		_updateLate(dt);
		

		{
			PROFILE_SCOPE("Timers");
			m_baseTimers.update(dt);
		}

		if (m_vkCtx) {
			VulkanContext::DrawContext drawCtx{};
			drawCtx.frameCount = static_cast<uint32_t>(m_totalFrames);
			{
				PROFILE_SCOPE("PreDraw");
				m_vkCtx->preDraw(getRenderManager());
			}

			{
				PROFILE_SCOPE("Draw");
				m_vkCtx->draw(drawCtx);
			}

			//TODO PRESENT HERE!

			{
				PROFILE_SCOPE("PostDraw");
				m_vkCtx->postDraw();
			}
		}

		m_framesSinceLastFpsRead++;
		m_totalFrames++;
		//std::this_thread::yield();

	}

	for (const auto scene : m_scenes) {
		scene->unloadDispatch();
		delete scene;
	}
	m_scenes.clear();

	//m_wnd->destroyWindow();

	

	m_status = EngineStatus::Stopped;
	onStopped.notify(this);
	LOG(LogType::Success, "Done.");
}

void Engine::stop() {
	LOGLINE(LogType::Info, LogMod::Engine, "Stopping... ");
	m_status = EngineStatus::PendingStop;
	onPendingStop.notify(this);
}

Camera* Engine::mainCamera() const {

	if (m_mainCamIndex.invalid())
		return nullptr;

	return &getScene(m_mainCamIndex.sceneIndex)->
		gameObjectSystem().getAllOfType<Camera>()[m_mainCamIndex.camIndex];
}

void Engine::setMainCamera(const uint16_t sceneIndex, const uint32_t camIndex) {
	const auto scene = getScene(sceneIndex);
	if (!scene)
		throw std::invalid_argument("No Such scene!");

	const auto& cams = scene->gameObjectSystem().getAllOfType<Camera>();
	if (cams.empty() || camIndex >= cams.size())
		throw std::invalid_argument("Scene has no cameras!");

	m_mainCamIndex = CamIndex{ sceneIndex, camIndex };
	
}

ErrorCode Engine::init(WindowContext* wndCtx) {

	if (wndCtx)
		m_wnd = wndCtx;

	ErrorCode EC{};
	EC_CHECK(EC, _initGraphics());

	EC_CHECK(EC, _initShaderManager());
	EC_CHECK(EC, _initInputManager());
	
	EC_CHECK(EC, _initBaseShaders());
	EC_CHECK(EC, _initBaseCallbacks());

	EC_CHECK(EC, _initJobSystem());


	return ErrorCode::OK;
}

inline void Engine::_updateEarly(double dt) {
	PROFILE_SCOPE("EarlyUpdate");
	onEarlyUpdateEnter.notify(this);
	for (auto& index : m_activeSceneIndices) {
		if (m_scenes[index]->isFirstFrame())
			m_scenes[index]->startDispatch();

		{
			PROFILE_SCOPE("Scene_EarlyUpdate");
			m_scenes[index]->updateDispatch(dt);
		}

		{
			PROFILE_SCOPE("SceneTransforms");
			m_scenes[index]->m_transformSys.run(m_scenes[index]->m_boundingSys);

		}

		{
			PROFILE_SCOPE("BoundingSystemUpdate");
			m_scenes[index]->m_boundingSys.run();
		}

		{
			PROFILE_SCOPE("SceneGameObjects");
			m_scenes[index]->m_gameObjectSys.run();
		}
		

		{
			PROFILE_SCOPE("Scene_BVH_Update");
			m_scenes[index]->bvhSystem().updateBVH(m_scenes[index]->m_reg, m_updateDeltaTime, 0.01667f);
		}
	}
	onEarlyUpdateExit.notify(this);
}

inline void Engine::_updateLate(const double dt) {
	PROFILE_SCOPE("LateUpdate");
	onLateUpdateEnter.notify(this);
	std::vector<uint32_t> removeIndices;
	for (auto& index : m_activeSceneIndices) {
		{
			PROFILE_SCOPE("Scene_LateUpdate");
			m_scenes[index]->lateUpdateDispatch(dt);
		}
		if (m_scenes[index]->pendingUnload()) {
			m_scenes[index]->unloadDispatch();
			delete m_scenes[index];
			auto it = m_scenes.begin() + index;
			m_scenes.erase(it);
			removeIndices.push_back(index);
		} else {
			{
				PROFILE_SCOPE("Scene_BVH_OcclusionPass");
				m_scenes[index]->cullResults.clear();

				m_scenes[index]->
					bvhSystem().performFrustumCullingWithOcclusion(
						m_scenes[index]->cullResults, mainCamera(), &m_scenes[index]->registry());
			}
			m_scenes[index]->bvhSystem().bvh().startRebuild(m_scenes[index]->registry());
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