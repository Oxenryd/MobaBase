#ifndef ENGINE_H
#define ENGINE_H

#ifdef BUILD_WIN
	#ifndef _INC_WINAPIFAMILY
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#include <windows.h>
	#endif
#endif


#include "Format_fixes.hpp"
#include "VulkanContext.hpp"

#include <cstdint>
#include <type_traits>
#include <list>
#include <algorithm>
#include <utility>

#include "WindowContext.h"
#include "InputManager.hpp"
#include "Debug.hpp"
#include "Delegate.hpp"
#include "ArenaAllocator.hpp"
#include "TimerSystem.h"
#include "RenderManager.h"
#include "Scene.h"
#include "GlobalSystem.hpp"
#include "MRandom.hpp"
#include "MWork.hpp"


constexpr double			DEFAULT_DELTATIME_JITTER_SETTING = 0.002;
constexpr double			FPS_400 = 1.0 / 400.0;
constexpr double			FPS_165 = 1.0 / 165.0;
constexpr double			FPS_120 = 1.0 / 120.0;
constexpr double			FPS_60 = 1.0 / 60.0;
constexpr double			FPS_50 = 1.0 / 50.0;
constexpr static double	MAX_DELTA_TIME = 1.0;

enum class EngineStatus : uint8_t
{
	Stopped,
	PendingRun,
	Running,
	PendingStop,
};

struct CamIndex
{
	uint16_t sceneIndex;
	uint32_t camIndex;
	CamIndex() : sceneIndex(UINT16_INVALID), camIndex(UINT32_INVALID) {}
	explicit CamIndex(const CamIndex& camIndex) :
		sceneIndex{camIndex.sceneIndex},
		camIndex{camIndex.camIndex}
	{}
	explicit CamIndex(const uint64_t raw) :
		sceneIndex{static_cast<uint16_t>(raw & 0xFFFF)},
		camIndex{static_cast<uint32_t>(raw & 0xFFFFFFFF << 16)}
	{}
	CamIndex(const uint16_t scene, const uint32_t cameraIndex) :
		sceneIndex{scene},
		camIndex{cameraIndex}
	{}

	bool invalid() const {
		return sceneIndex == UINT16_INVALID || camIndex == UINT32_INVALID;
	}

	explicit operator bool() const {
		return !invalid();
	}

	bool operator==(const CamIndex& rhs) const {
		return sceneIndex == rhs.sceneIndex && camIndex == rhs.camIndex;
	}
};

class Engine
{
	static inline Engine* s_instance = nullptr;

	HeapArena m_baseArena;
	std::string m_appName;
	std::vector<SceneBase*> m_scenes;
	std::set<uint32_t> m_activeSceneIndices;
	uint16_t m_newSceneIndex = 0;
	double m_targetUpdateDeltaTime{};
	double m_updateDeltaTime{};
	double m_targetFixedDeltaTime{};
	double m_fixedAccu{};
	uint32_t m_framesSinceLastFpsRead{};
	EngineStatus m_status = EngineStatus::Stopped;
	bool m_sceneTransitionRequested = false;
	SceneTransitionMode m_sceneTransitMode = SceneTransitionMode::WaitForDone;
	double m_lastReadFps{};
	float m_baseCosF{};
	float m_baseSinF{};
	double m_baseCosD{};
	double m_baseSinD{};
	size_t m_requestedSceneIndex = SIZE_INVALID;
	uint32_t m_nextTraceId = 0;
	uint32_t m_framesToTrace = 3;
	uint32_t m_framesTraced = UINT32_INVALID;
	bool m_pendingTrace = false;
	
	size_t m_totalFrames = 0;
	double m_totalTime = 0.0;
	std::chrono::steady_clock::time_point m_lastUpdateTime;
	CamIndex m_mainCamIndex{ UINT16_INVALID, UINT32_INVALID };


	INLINE double _tickDt();
	INLINE void _updateEarly(double dt);
	INLINE void _updateLate(double dt);
	INLINE void _updateFixed();
	INLINE void _run();
	INLINE ErrorCode _initShaderManager();
	INLINE ErrorCode _initInputManager();
	INLINE ErrorCode _initGraphics();
	INLINE ErrorCode _initBaseShaders() const;
	INLINE ErrorCode _initBaseCallbacks();
	INLINE ErrorCode _initJobSystem();

	// Base Systems
	RenderManager* m_renderMan = nullptr;
	WindowContext* m_wnd = nullptr;
	InputManager* m_inputMan = nullptr;
	VulkanContext* m_vkCtx = nullptr;
	FrameArena* m_workArena = nullptr;


	GlobalSystem m_globalSystem;

	TimerSystem m_baseTimers;
	Timer m_fpsCountTimer;

public:
	~Engine();
	Engine(const char* appName, size_t heapSize);
	
	Camera* mainCamera() const;

	// EVENTS
	Event<Engine*> onStartInitiated;
	Event<Engine*> onStarted;
	Event<Engine*> onBeginRunning;
	Event<Engine*> onPendingStop;
	Event<Engine*> onStopped;
	Event<Engine*> onEarlyUpdateEnter;
	Event<Engine*> onEarlyUpdateExit;
	Event<Engine*> onLateUpdateEnter;
	Event<Engine*> onLateUpdateExit;
	Event<Engine*> onFixedUpdateEnter;
	Event<Engine*> onFixedUpdateExit;
	Event<Engine*, uint32_t> onReadFPS;

	template <SceneConcept T>	
	INLINE SceneBase* createNewScene(size_t arenaSize, void* argument) {
		uint16_t index = m_newSceneIndex++;
		m_scenes.resize(std::max(static_cast<size_t>(index), m_scenes.size() + 1));
		m_scenes[index] = T::createDefault(arenaSize, index, argument);
		return m_scenes[index];
	}

	INLINE ErrorCode registerActiveScene(SceneBase* const scene) {
		const auto it = m_activeSceneIndices.find({ scene->m_sceneIndex });
		if (it != m_activeSceneIndices.end()) {
			return ErrorCode::SCENE_ALREADY_ACTIVE;
		}
		m_activeSceneIndices.insert({ scene->m_sceneIndex });
		return ErrorCode::OK;
	}
	INLINE ErrorCode unregisterActiveScene(SceneBase* const scene) {
		const auto it = m_activeSceneIndices.find({ scene->m_sceneIndex });
		if (it == m_activeSceneIndices.end()) {
			return ErrorCode::SCENE_UNTRACKED_SCENE_REFERENCE;
		}
		m_activeSceneIndices.erase(scene->m_sceneIndex);
		return ErrorCode::OK;
	}
	INLINE ArenaRegistry& getSceneRegistry(const SceneBase* const scene) const {
		return m_scenes[scene->m_sceneIndex]->registry();
	}

	INLINE void armFrameTrace() { m_pendingTrace = true; }
	INLINE GlobalSystem& getGlobalSystem() { return m_globalSystem; }
	INLINE HeapArena& baseArena() { return m_baseArena; }
	INLINE double deltaTime() const { return m_updateDeltaTime; }
	INLINE double fixedDeltaTime() const { return m_targetFixedDeltaTime; }
	INLINE double totalTime() const { return m_totalTime; }
	INLINE size_t totalFrames() const { return m_totalFrames; }
	INLINE void setTargetUpdateDeltaTime(const double targetDt) { m_targetUpdateDeltaTime = targetDt; }
	INLINE static double sin() { return s_instance->m_baseSinD; }
	INLINE static double cos() { return s_instance->m_baseCosD; }
	INLINE static float sinF() { return s_instance->m_baseSinF; }
	INLINE static float cosF() { return s_instance->m_baseCosF; }
	INLINE RenderManager* getRenderManager() const { return m_renderMan; }
	void start();
	void stop();
	INLINE const EngineStatus& getStatus() const { return m_status; }
	INLINE SceneBase* getScene(const size_t index) const { return m_scenes[index]; }
	INLINE std::vector<SceneBase*> getActiveScenes() const {
		std::vector<SceneBase*> scenes;
		for (auto& index : m_activeSceneIndices) {
			scenes.push_back(m_scenes[index]);
		}
		return scenes;
	}
	INLINE void requestSceneTransition(const SceneTransitionMode mode, const size_t requestedSceneIndex) {
		if (!m_sceneTransitionRequested) {
			m_sceneTransitionRequested = true;
			m_sceneTransitMode = mode;
			m_requestedSceneIndex = requestedSceneIndex;
		}
	}

	void setMainCamera(uint16_t sceneIndex, uint32_t camIndex);
	ErrorCode init(WindowContext* wndCtx = nullptr);
	INLINE WindowContext* getWndSurface() const { return m_wnd; }
	INLINE InputManager* getInputManager() const { return m_inputMan; }
	INLINE VulkanContext* getVulkanContext() const { return m_vkCtx; }

	static Engine* getInstance() { return s_instance; }
};

#endif //ENGINE_H