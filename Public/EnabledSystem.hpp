//#ifndef ENABLED_SYSTEM_HPP
//#define ENABLED_SYSTEM_HPP
//
//#include <entt/entt.hpp>
//#include "EnabledTag.hpp"
//#include "SystemECS.h"
//
//class EnabledSystem : public SystemECS
//{
//public:
//	virtual ~EnabledSystem() {};
//	EnabledSystem() = delete;
//	EnabledSystem(entt::registry* const registry)
//		: SystemECS{registry}
//	{}
//	void registryEmplace(entt::entity entity, void* valuePtr, void* valueOut) override {
//		auto& tag = m_reg->emplace_or_replace<EnabledTagComponent>(entity, EnabledTagComponent{0});
//	}
//	void registryRemove(entt::entity entity, void* valueOut) override {
//		m_reg->remove<EnabledTagComponent>(entity);
//	}
//	void run() override {}
//};
//
//#endif