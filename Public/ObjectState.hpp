#ifndef OBJECTSTATE_HPP
#define OBJECTSTATE_HPP

#include <cstdint>
#include <Bits.hpp>

#define NONE 0x00
#define DIRTY_TRANSFORM 0x01

enum class ObjectState : uint8_t
{
	None			= NONE,
	DirtyTransform	= DIRTY_TRANSFORM
};

#undef NONE
#undef DIRTY_TRANSFORM
#endif