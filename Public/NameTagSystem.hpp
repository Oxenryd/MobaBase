#ifndef NAMETAG_SYSTEM_HPP
#define NAMETAG_SYSTEM_HPP

#include "SystemECS.h"
#include "NameTag.hpp"

class NameTagSystem : public SystemECS
{
private:

public:
	virtual ~NameTagSystem() {}
	NameTagSystem() = delete;
	NameTagSystem(entt::registry* const registry)
		: SystemECS{registry}
	{}

	virtual void registryEmplace(entt::entity entity, void* valuePtr, void* valueOut) {
		auto& name = deref<std::string>(valuePtr);
		m_reg->emplace_or_replace<NameTagComponent>(entity, NameTagComponent{ name });
	}
	virtual void registryRemove(entt::entity entity, void* valueOut) {
		m_reg->remove<NameTagComponent>(entity);
	}
};

#endif