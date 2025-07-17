#ifndef MATERIALSTATES_H
#define MATERIALSTATES_H

#include <cstdint>

enum class BlendMode : uint8_t
{
	Opaque,
	Alpha,
	Premultiplied,
	Additive,
	Custom
};

enum class DepthMode : uint8_t
{
	DepthDefault,
	ReadOnly,
	None,
	Custom
};

enum class RasterMode : uint8_t
{
	RasterDefault,
	WireFrame,
	NoCulling,
	Custom
};

enum class MultiSamplingMode : uint8_t
{
	MSAA_None,
	MSAA_4x
};

#endif