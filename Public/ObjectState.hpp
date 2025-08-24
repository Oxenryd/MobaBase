#ifndef OBJECTSTATE_HPP
#define OBJECTSTATE_HPP

#include <cstdint>
#include <Bits.hpp>
#include "GlobalMacros.h"

#define ObjectStateType uint16_t
#define StateField SizedBitField<ObjectStateType>

enum class ObjectState : ObjectStateType
{
	None					= 0x00,
	DirtyTransform			= 0x01,
	IgnoreParentTransform   = 0x02,
	LocalEnableOverride		= 0x04,
	MovedThisFrame			= 0x08,
	ParentMovedThisFrame	= 0x10,
	TranslationDirty		= 0x20,
	RotationDirty			= 0x40,
	ScaleDirty				= 0x80
};


INLINE bool objectState_is_dirty(ObjectStateType mask) {
	return  (mask & static_cast<ObjectStateType>(ObjectState::DirtyTransform));
}

INLINE bool objectState_only_translation_dirty(ObjectStateType mask) {
	return	
			(mask & static_cast<ObjectStateType>(ObjectState::TranslationDirty)) &&
			!( 
				(mask & static_cast<ObjectStateType>(ObjectState::RotationDirty)) ||
				(mask & static_cast<ObjectStateType>(ObjectState::ScaleDirty))
			);
}

#endif