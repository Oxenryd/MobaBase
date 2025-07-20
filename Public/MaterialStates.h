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

enum class SamplerAddressMode : uint8_t
{
	Repeat = 0,
	MirroredRepeat = 1,
	ClampEdge = 2,
	ClampBorder = 3,
};

enum class SamplerMode : uint8_t
{
	Point,
	Linear,
	Aniso4X,
	Aniso8X,
	Aniso16X
};

struct SamplerState
{
	SamplerMode sampler = SamplerMode::Point;
	SamplerAddressMode addressMode = SamplerAddressMode::Repeat;

	friend bool operator==(const SamplerState& lhs, const SamplerState& rhs) {
		return lhs.addressMode == rhs.addressMode && lhs.sampler == rhs.sampler;
	}
	friend bool operator!=(const SamplerState& lhs, const SamplerState& rhs) {
		return !(lhs == rhs);
	}
};




struct SamplerStateHash
{
private:
	inline void hash_combine(size_t& seed, uint8_t val) const {
		seed ^= std::hash<uint8_t>{}(val)+0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
public:
	size_t operator()(const SamplerState& s) const {
		size_t seed = 0;
		hash_combine(seed, static_cast<uint8_t>(s.sampler));
		hash_combine(seed, static_cast<uint8_t>(s.addressMode));
		return seed;
	}
};


#endif