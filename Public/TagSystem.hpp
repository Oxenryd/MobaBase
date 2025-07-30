#ifndef TAG_SYSTEM_HPP
#define TAG_SYSTEM_HPP

#include "SystemECS.h"
#include "Tag.hpp"

class TagSystem : public SystemECS
{
private:

public:
	virtual ~TagSystem() {}
	TagSystem() = delete;
	TagSystem(ArenaRegistry* const registry, uint16_t sceneIndex)
		: SystemECS{registry, sceneIndex}
	{}

	virtual void registryEmplace(entt::entity entity, void* valuePtr, void* valueOut) {
		auto& tag = deref<std::string>(valuePtr);
		m_reg->emplace_or_replace<TagComponent>(entity, TagComponent{ tag });
	}
	virtual void registryRemove(entt::entity entity, void* valueOut) {
		m_reg->remove<TagComponent>(entity);
	}
};

namespace entt
{
	template<>
	struct storage_type<TagComponent, ArenaRegistry>
	{
		using type = ArenaStorage<TagComponent>;
	};
}


#endif