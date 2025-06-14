#ifndef FSM_hpp
#define FSM_hpp
#pragma once

#include <vector>
#include <cstdint>
#include <concepts>

template<typename T>
concept HasOnEnter = requires(T t, void* ptr) {
	{ t.onEnter(ptr) };
};

template<typename T>
concept HasOnExit = requires(T t, void* ptr) {
	{ t.onExit(ptr) };
};

template<typename T>
concept HasOnTransition = requires(T t, void* ptr) {
	{ t.onTransition(ptr) };
};

template<typename T>
concept HasOnCompleted = requires(T t, void* ptr) {
	{ t.onCompleted(ptr) };
};

template<typename T>
concept HasRun = requires(T t, void* ptr) {
	{ t.run(ptr) };
};


enum class TransitionStatus : uint8_t
{
	Waiting,
	Running,
	Completed
};

enum class FsmStateStatus : uint8_t
{
	Waiting,
	Running,
	Transitioning,
	Completed
};

class FsmStateTransitionBase;

class FsmStateBase
{
public:
	virtual ~FsmStateBase() = default;
	virtual FsmStateStatus runBase(void* context) = 0;
	void transit(FsmStateTransitionBase* const nextTransition) {
		m_nextTransition = nextTransition;
		m_status = FsmStateStatus::Transitioning;
	}
	const FsmStateStatus& status() const { return m_status; }
	FsmStateTransitionBase* const nextTransition() const { return m_nextTransition; }

protected:
	FsmStateTransitionBase* m_nextTransition = nullptr;
	FsmStateStatus m_status = FsmStateStatus::Waiting;
};


class FsmStateTransitionBase
{
public:
	virtual ~FsmStateTransitionBase() = default;
	virtual TransitionStatus runBase(void* context) = 0;
	void transit(FsmStateBase* nextState) {
		m_nextState = nextState;
		m_status = TransitionStatus::Completed;
	}
	const TransitionStatus& status() const { return m_status; }
	FsmStateBase* const nextState() const { return m_nextState; }
protected:
	FsmStateBase* m_nextState = nullptr;
	TransitionStatus m_status = TransitionStatus::Waiting;
};


template <typename TransitionDerived>
class FsmStateTransition : public FsmStateTransitionBase
{
public:
	TransitionStatus runBase(void* context) override {
		if (m_status == TransitionStatus::Waiting)
			onEnterBase(context);
		m_status = TransitionStatus::Running;

		if constexpr (HasRun<TransitionDerived>)
			static_cast<TransitionDerived*>(this)->run(context);

		if (m_status == TransitionStatus::Completed) {
			m_status = TransitionStatus::Waiting;
			onExitBase(context);
			return TransitionStatus::Completed;
		}
		return m_status;
	}
protected:
	void onEnterBase(void* context) {
		if constexpr (HasOnEnter<TransitionDerived>)
			static_cast<TransitionDerived*>(this)->onEnter(context);
	}
	void onExitBase(void* context) {
		if constexpr (HasOnExit<TransitionDerived>)
			static_cast<TransitionDerived*>(this)->onExit(context);
	}
};


template <typename StateDerived>
class FsmState : public FsmStateBase
{
public:
	FsmStateStatus runBase(void* context) override {
		switch (m_status) {
			case FsmStateStatus::Waiting:
				onEnterBase(context);
				m_status = FsmStateStatus::Running;

				if constexpr (HasRun<StateDerived>)
					static_cast<StateDerived*>(this)->run(context);

				return m_status;

			case FsmStateStatus::Running:
				if constexpr (HasRun<StateDerived>)
					static_cast<StateDerived*>(this)->run(context);
				return m_status;

			case FsmStateStatus::Transitioning:
				onTransitionBase(context);
				TransitionStatus transResult =
					m_nextTransition->runBase(context);
				switch (transResult) {
					case TransitionStatus::Waiting:
					case TransitionStatus::Running:
						m_status = FsmStateStatus::Transitioning;
						return m_status;
					case TransitionStatus::Completed:
						onTransitionCompletedBase(context);
				}
		}
		m_status = FsmStateStatus::Waiting;
		return FsmStateStatus::Completed;
	}
protected:
	void onEnterBase(void* context) {
		if constexpr (HasOnEnter<StateDerived>)
			static_cast<StateDerived*>(this)->onEnter(context);
	}
	void onTransitionBase(void* context) {
		if constexpr (HasOnTransition<StateDerived>)
			static_cast<StateDerived*>(this)->onTransition(context);
	}
	void onTransitionCompletedBase(void* context) {
		if constexpr (HasOnCompleted<StateDerived>)
			static_cast<StateDerived*>(this)->onCompleted(context);
	}
};


class Fsm
{
public:
	Fsm() {}

	void runBase(void* context) {
		if (!m_baseState || !m_currentState)
			return;

		FsmStateStatus result = m_currentState->runBase(context);
		if (result == FsmStateStatus::Completed) {
			m_pendingState =
				m_currentState->nextTransition()->nextState();
			if (m_pendingState != nullptr) {
				m_currentState = m_pendingState;
				m_pendingState = nullptr;
			} else
				m_currentState = m_baseState;
		}
	}
	const FsmStateBase* currentState() const { return m_currentState; }

protected:
	FsmStateBase* m_baseState = nullptr;
	FsmStateBase* m_currentState = nullptr;
	FsmStateBase* m_pendingState = nullptr;
};


#endif