#ifndef ENGINE_H
#define ENGINE_H

#ifdef BUILD_WIN
	#ifndef _INC_WINAPIFAMILY
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#include <windows.h>
	#endif
#endif

#ifdef USE_VULKAN
	#include "VulkanContext.hpp"
#endif

#include <cstdint>

#include "WindowSurface.h"
#include "InputManager.hpp"
#include "Debug.hpp"
#include "Log.hpp"
#include "Delegate.hpp"

constexpr const double			DEFAULT_DELTATIME_JITTER_SETTING = 0.002;
constexpr const double			FPS_400 = 1.0 / 400.0;
constexpr const double			FPS_165 = 1.0 / 165.0;
constexpr const double			FPS_120 = 1.0 / 120.0;
constexpr const double			FPS_60 = 1.0 / 60.0;
constexpr const double			FPS_50 = 1.0 / 50.0;
constexpr static const double	MAX_DELTA_TIME = 0.25;

struct EngineSettings
{
	WindowSurface* mainWindow = nullptr;
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
	EngineSettings m_options;
	EngineStatus m_status = EngineStatus::Stopped;
	double m_targetUpdateDeltaTime;
	double m_updateDeltaTime;
	double m_targetFixedDeltaTime;
	double m_fixedAccu;
	std::chrono::steady_clock::time_point m_lastUpdateTime;

	inline double _tickDt();
	inline void _updateEarly(double dt);
	inline void _updateLate(double dt);
	inline void _updateFixed();
	inline void _run();

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

	inline explicit Engine() :
		m_targetUpdateDeltaTime{ FPS_400 },
		m_updateDeltaTime{ 0.0 },
		m_lastUpdateTime{ std::chrono::steady_clock::now() },
		m_options{},
		m_targetFixedDeltaTime{ FPS_60 },
		m_fixedAccu{ 0.0 }
	{}
	
	inline const double& deltaTime() { return m_updateDeltaTime; }
	inline void setTargetUpdateDeltaTime(const double targetDt) { m_targetUpdateDeltaTime = targetDt; }
	void start(WindowSurface* wndPtr, InputManager* inputPtr);
	inline void stop();
	
};

#endif //ENGINE_H