#ifndef BOUNDINGSYSTEM_H
#define BOUNDINGSYSTEM_H

#include "SystemECS.h"
#include "BasicTypes.hpp"


class BoundingSystem : public SystemECS
{
private:
	ArenaVector<BSphere> m_bSpheres;
	ArenaVector<AABB> m_aabbs;
	ArenaVector<OBB> m_obbs;

public:
	BoundingSystem(ArenaRegistry* const registry, Arena* const arena, uint16_t sceneIndex) :
		SystemECS{registry, sceneIndex},
		m_bSpheres{ ArenaAllocator<BSphere>{arena} },
		m_aabbs{ ArenaAllocator<AABB>{arena} },
		m_obbs{ ArenaAllocator<OBB>{arena} }
	{
	}


	virtual void registryEmplace(entt::entity entity, void* valuePtr, void* valueOut) override {
		
		BSphere* sphere = static_cast<BSphere*>(valuePtr);

		auto index = static_cast<uint32_t>(m_bSpheres.size());
		if (!sphere)
			m_bSpheres.push_back({});
		else {
			m_bSpheres.push_back(*sphere);
		}

		auto& comp = m_reg->emplace_or_replace<BoundingVolumeComponent>(entity, BoundingVolumeComponent{ index });

		if (valueOut) {
			*static_cast<BoundingVolumeComponent*>(valueOut) = comp;
		}
	}
	virtual void registryRemove(entt::entity, void* valueOut) override {
		throw std::exception("SystemECS::registryRemove() not implemented.");
	}

	ArenaVector<BSphere>& bSpheres() { return m_bSpheres; }
};

#endif