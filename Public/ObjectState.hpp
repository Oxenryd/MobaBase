#ifndef OBJECTSTATE_HPP
#define OBJECTSTATE_HPP

#include <cstdint>
#include <Bits.hpp>

#define ObjectStateType uint16_t
#define StateField SizedBitField<ObjectStateType>

enum class ObjectState : ObjectStateType
{
	None					= 0x00,
	DirtyTransform			= 0x01,
	IgnoreParentTransform   = 0x02,
	LocalEnableOverride		= 0x04,
	MovedThisFrame			= 0x08,
	ParentMovedThisFrame	= 0x10
};


#endif