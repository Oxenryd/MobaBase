#ifndef BOUNDINGSYSTEM_H
#define BOUNDINGSYSTEM_H

#include "SystemECS.h"
#include "BasicTypes.hpp"

constexpr const uint32_t AaBbTriIndices[36]{
	// Front  (+Z)
	0,2,1,  1,2,3,
	// Back   (-Z)
	5,7,4,  4,7,6,
	// Left   (-X)
	4,6,0,  0,6,2,
	// Right  (+X)
	1,3,5,  5,3,7,
	// Top    (+Y)
	4,0,5,  5,0,1,
	// Bottom (-Y)
	2,6,3,  3,6,7
};

class BoundingSystem : public SystemECS
{
private:
	ArenaVector<BSphere> m_bSpheres;
	ArenaVector<AABB> m_aabbs;
	ArenaVector<OBB> m_obbs;
	ArenaVector<AABB> m_aabbLocals;

public:
	BoundingSystem(ArenaRegistry* const registry, Arena* const arena, uint16_t sceneIndex) :
		SystemECS{registry, sceneIndex},
		m_bSpheres{ ArenaAllocator<BSphere>{arena} },
		m_aabbs{ ArenaAllocator<AABB>{arena} },
		m_obbs{ ArenaAllocator<OBB>{arena} },
		m_aabbLocals{ ArenaAllocator<AABB>{arena} }
	{
	}



	virtual void registryEmplace(entt::entity entity, void* valuePtr = nullptr, void** valueOut = nullptr) override {
		
		AABB* box = static_cast<AABB*>(valuePtr);

		const auto indexLocal = static_cast<uint32_t>(m_aabbLocals.size());
		const auto indexWorld = static_cast<uint32_t>(m_aabbs.size());
		if (!box) {
			m_aabbLocals.push_back({});
			m_aabbs.push_back({});
		}
		else {
			m_aabbLocals.push_back(*box);
			m_aabbs.push_back(*box);
		}

		auto& comp = m_reg->emplace_or_replace<BoundingVolumeComponent>(
			entity, BoundingVolumeComponent{ indexLocal, indexWorld });
		m_reg->emplace_or_replace<EnabledTag>(entity);

		if (valueOut) {
			*valueOut = static_cast<void*>(&comp);
			//*static_cast<BoundingVolumeComponent*>(valueOut) = comp;
		}
	}
	virtual void registryRemove(entt::entity, void* valueOut) override {
		throw std::exception("SystemECS::registryRemove() not implemented.");
	}

	ArenaVector<BSphere>& bSpheres() { return m_bSpheres; }
	ArenaVector<AABB>& aabbs() { return m_aabbs; }
	ArenaVector<OBB>& obbs() { return m_obbs; }
	ArenaVector<AABB>& cachedLocals() { return m_aabbLocals; }
};

#endif