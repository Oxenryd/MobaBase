#ifndef ENGINE_H
#define ENGINE_H

#include "ArenaAllocator.hpp"
#include "Delegate.hpp"
#ifdef BUILD_WIN
	#ifndef _INC_WINAPIFAMILY
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#include <windows.h>
	#endif
#endif


#include <cstdint>
#include <algorithm>
#include <set>
#include <memory>

#include "Concepts.h"
#include "GlobalMacros.h"
#include "ErrorCodes.hpp"

// Forwards
enum class SceneTransitionMode : uint8_t;
class RenderManager;
class WindowContext;
class InputManager;
class VulkanContext;
class SceneBase;
class Camera;
class HeapArena;
class FrameArena;
class GlobalSystem;
class TimerSystem;
class Timer;


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

	// Base Arenas
	HeapArena* m_baseArena;
	FrameArena* m_jobsArena;

	// Context
	std::unique_ptr<WindowContext> m_wnd{nullptr};

	// Base Systems
	TimerSystem* m_baseTimers;
	Timer* m_fpsCountTimer;
	InputManager* m_inputMan = nullptr;
	RenderManager* m_renderMan = nullptr;
	VulkanContext* m_vkCtx = nullptr;
	GlobalSystem* m_globalSystem;

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
	SceneTransitionMode m_sceneTransitMode;
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
	ErrorCode m_currentEC;
	size_t m_totalFrames = 0;
	double m_totalTime = 0.0;
	std::chrono::steady_clock::time_point m_lastUpdateTime{ std::chrono::steady_clock::now() };
	CamIndex m_mainCamIndex{ UINT16_INVALID, UINT32_INVALID };

	INLINE double _tickDt();
	INLINE void _updateEarly(double dt);
	INLINE void _updateLate(double dt);
	INLINE void _updateFixed();
	INLINE void _run();
	INLINE ErrorCode _init(size_t heapSize = 256_MB, bool editor = false);
	INLINE ErrorCode _initVulkan();
	INLINE ErrorCode _initRendering();
	INLINE ErrorCode _initBaseShaders() const;
	INLINE ErrorCode _initArenas(size_t heapSize);
	INLINE ErrorCode _initBaseSystems();


public:
	~Engine();
	Engine() = delete;
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	Engine& operator=(Engine&&) = delete;
	explicit Engine(
		std::unique_ptr<WindowContext> wndCtx,
		const std::string& appName,
		size_t heapSize = 256_MB,
		bool editor = false);
	
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
	INLINE SceneBase* createScene(size_t arenaSize, void* argument) {
		uint16_t index = m_newSceneIndex++;
		m_scenes.resize(std::max(static_cast<size_t>(index), m_scenes.size() + 1));
		m_scenes[index] = T::create(arenaSize, index, argument);
		return m_scenes[index];
	}

	ErrorCode registerActiveScene(SceneBase* const scene);
	INLINE ErrorCode unregisterActiveScene(SceneBase* const scene);
	INLINE ArenaRegistry& getSceneRegistry(const SceneBase* const scene) const;

	INLINE void armFrameTrace() { m_pendingTrace = true; }
	INLINE GlobalSystem& getGlobalSystem() { return *m_globalSystem; }
	INLINE HeapArena& baseArena() { return *m_baseArena; }
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
	INLINE WindowContext* getWindowContext() const { return m_wnd.get(); }
	INLINE InputManager* getInputManager() const { return m_inputMan; }
	INLINE VulkanContext* getVulkanContext() const { return m_vkCtx; }
	INLINE ErrorCode getCurrentError() const { return m_currentEC; }

	static Engine* getInstance() { return s_instance; }
};

#endif //ENGINE_H