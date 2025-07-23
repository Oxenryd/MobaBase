#ifndef ENGINE_H
#define ENGINE_H

#ifdef BUILD_WIN
	#ifndef _INC_WINAPIFAMILY
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#include <windows.h>
	#endif
#endif


#include "VulkanContext.hpp"


#include <cstdint>
#include <type_traits>
#include <list>
#include <map>
#include <algorithm>

#include "WindowSurface.h"
#include "InputManager.hpp"
#include "Debug.hpp"
#include "Log.hpp"
#include "Delegate.hpp"
#include "ArenaAllocator.hpp"
#include "Timer.h"
#include "TimerSystem.h"
#include "RenderManager.h"
#include "Scene.h"


constexpr const double			DEFAULT_DELTATIME_JITTER_SETTING = 0.002;
constexpr const double			FPS_400 = 1.0 / 400.0;
constexpr const double			FPS_165 = 1.0 / 165.0;
constexpr const double			FPS_120 = 1.0 / 120.0;
constexpr const double			FPS_60 = 1.0 / 60.0;
constexpr const double			FPS_50 = 1.0 / 50.0;
constexpr static const double	MAX_DELTA_TIME = 0.25;

class RenderManager;
struct EngineSettings
{
	VulkanContext* graphicContext = nullptr;
	InputManager* inputManager = nullptr;
	double deltaTimeJitterThreshold = DEFAULT_DELTATIME_JITTER_SETTING;
};

enum class EngineStatus : uint8_t
{
	Stopped,
	PendingRun,
	Running,
	PendingStop,
};

class Engine
{
private:
	static inline Engine* s_instance = nullptr;

	HeapArena& m_baseArena;
	EngineSettings m_options;
	std::vector<SceneBase*> m_scenes;
	std::set<uint32_t> m_activeSceneIndices;
	size_t m_newSceneIndex = 0;
	//size_t m_curSceneIndex;
	double m_targetUpdateDeltaTime;
	double m_updateDeltaTime;
	double m_targetFixedDeltaTime;
	double m_fixedAccu;
	uint32_t m_framesSinceLastFpsRead;
	EngineStatus m_status = EngineStatus::Stopped;
	double m_lastReadFps;
	bool m_sceneTransitionRequested = false;
	size_t m_requestedSceneIndex = static_cast<size_t>(-1);
	SceneTransitionMode m_sceneTransitMode = SceneTransitionMode::WaitForDone;
	uint64_t m_totalFrames = 0;
	std::chrono::steady_clock::time_point m_lastUpdateTime;


	inline double _tickDt();
	inline void _updateEarly(double dt);
	inline void _updateLate(double dt);
	inline void _updateFixed();
	inline void _run();
	void _initShaderManager();

	// Base Systems
	RenderManager* m_renderMan;
	TimerSystem m_baseTimers;
	Timer m_fpsCountTimer;

public:
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
	inline SceneBase* createNewScene(void* argument) {
		uint32_t index = m_newSceneIndex++;
		m_scenes.resize(std::max(static_cast<size_t>(index), m_scenes.size() + 1));
		m_scenes[index] = (T::createDefault(index, argument));
		return m_scenes[index];
	}

	inline ErrorCode registerActiveScene(SceneBase* const scene) {
		auto it = m_activeSceneIndices.find({ scene->m_sceneIndex });
		if (it != m_activeSceneIndices.end()) {
			return ErrorCode::SCENE_ALREADY_ACTIVE;
		}
		m_activeSceneIndices.insert({ scene->m_sceneIndex });
		return ErrorCode::OK;
	}
	inline ErrorCode unregisterActiveScene(SceneBase* const scene) {
		auto it = m_activeSceneIndices.find({ scene->m_sceneIndex });
		if (it == m_activeSceneIndices.end()) {
			return ErrorCode::SCENE_UNTRACKED_SCENE_REFERENCE;
		}
		m_activeSceneIndices.erase(scene->m_sceneIndex);
		return ErrorCode::OK;
	}
	inline ArenaRegistry& getSceneRegistry(SceneBase* const scene) {
		return m_scenes[scene->m_sceneIndex]->registry();
	}

	Engine(HeapArena& heap);
	~Engine();
	inline const double& deltaTime() { return m_updateDeltaTime; }
	inline void setTargetUpdateDeltaTime(const double targetDt) { m_targetUpdateDeltaTime = targetDt; }
	RenderManager* getRenderManager() { return m_renderMan; }
	void start(VulkanContext* graphicContext, InputManager* inputPtr);
	void stop();
	inline const EngineStatus& getStatus() const { return m_status; }
	inline SceneBase* const getScene(const size_t index) { return m_scenes[index]; }
	inline std::vector<SceneBase*> const getActiveScenes() { 
		std::vector<SceneBase*> scenes;
		for (auto& index : m_activeSceneIndices) {
			scenes.push_back(m_scenes[index]);
		}
		return scenes;
	}
	inline void requestSceneTransition(const SceneTransitionMode mode, const size_t requestedSceneIndex) {
		if (!m_sceneTransitionRequested) {
			m_sceneTransitionRequested = true;
			m_sceneTransitMode = mode;
			m_requestedSceneIndex = requestedSceneIndex;
		}
	}

	static Engine* getInstance() { return s_instance; }
};

#endif //ENGINE_H