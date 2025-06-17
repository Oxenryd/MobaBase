#include "Engine.h"
#include <immintrin.h>

Engine::Engine() :
	m_targetUpdateDeltaTime{ FPS_400 },
	m_updateDeltaTime{ 0.0 },
	m_lastUpdateTime{ std::chrono::steady_clock::now() },
	m_options{},
	m_targetFixedDeltaTime{ FPS_60 },
	m_fixedAccu{ 0.0 },
	m_framesSinceLastFpsRead{ 0 },
	m_baseTimers{ 1 },
	m_lastReadFps{m_targetUpdateDeltaTime},
	m_fpsCountTimer{ m_baseTimers.createTimer(true, 1.0) } {

	m_baseTimers.m_incOnTimes[m_fpsCountTimer.m_timerIndex].subscribe([this]()
		{
			onReadFPS.notify(this, m_framesSinceLastFpsRead);
			m_framesSinceLastFpsRead = 0;
		});
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

void Engine::start(WindowSurface* wndPtr, InputManager* inputPtr) {

	LOGLINE(LogType::Info, LogMod::Engine, "Starting... ");

	if (m_status != EngineStatus::Stopped) {
		LOGLINE(LogType::Error, LogMod::Engine, "Tried to start while not stopped. Ignoring.");
		return;
	}

	// Set main window
	m_options.inputManager = inputPtr;
	m_options.mainWindow = wndPtr;

	m_status = EngineStatus::PendingRun;
	onStartInitiated.notify(this);
	
	m_options.mainWindow->showWindow(SW_NORMAL);

	m_status = EngineStatus::Running;
	onStarted.notify(this);

	m_lastUpdateTime = std::chrono::steady_clock::now();

	LOG(LogType::Success, "Done.");
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

		m_options.inputManager->pollEvents();
		_updateEarly(dt);
		_updateLate(dt);

		m_baseTimers.update(dt);


		m_framesSinceLastFpsRead++;
		std::this_thread::yield();
	}

	m_options.mainWindow->destroyWindow();

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
	// TODO
	onEarlyUpdateExit.notify(this);
}

inline void Engine::_updateLate(double dt) {
	onLateUpdateEnter.notify(this);
	// TODO
	onLateUpdateExit.notify(this);
}

inline void Engine::_updateFixed() {
	onFixedUpdateEnter.notify(this);
	// TODO
	onFixedUpdateExit.notify(this);
}
