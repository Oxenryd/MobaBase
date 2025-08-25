#ifndef OBJECTSTATE_HPP
#define OBJECTSTATE_HPP

#include <cstdint>
#include <Bits.hpp>
#include "GlobalMacros.h"

#define ObjectStateType uint16_t
#define StateField SizedBitField<ObjectStateType>

enum class ObjectState : ObjectStateType
{
	None					= 0x0000,
	DirtyTransform			= 0x0001,
	IgnoreParentTransform   = 0x0002,
	LocalEnableOverride		= 0x0004,
	MovedThisFrame			= 0x0008,
	ParentMovedThisFrame	= 0x0010,
	TranslationDirty		= 0x0020,
	RotationDirty			= 0x0040,
	ScaleDirty				= 0x0080,
	DirtyChildren			= 0x0100
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