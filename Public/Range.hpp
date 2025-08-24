#ifndef RANGE_HPP
#define RANGE_HPP

#include <cstdint>
#include "GlobalMacros.h"

struct Range
{
	uint32_t offset;
	uint32_t count;

	INLINE size_t end() { return static_cast<size_t>(offset + count); }
	INLINE size_t start() { return static_cast<size_t>(offset); }
};

#endif