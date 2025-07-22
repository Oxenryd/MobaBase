#ifndef OBJECTSTATE_HPP
#define OBJECTSTATE_HPP

#include <cstdint>
#include <Bits.hpp>

#define NONE 0x00
#define ENABLED 0x01
#define DIRTY_TRANSFORM 0x02

enum class ObjectState : uint8_t
{
	None		= NONE,
	Enabled		= ENABLED,
	DirtyTransform		= DIRTY_TRANSFORM
};

#undef NONE
#undef ENABLED
#endif