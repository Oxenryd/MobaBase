#ifndef FRAMEDATA_HPP
#define FRAMEDATA_HPP

#include <vector>
#include <span>
#include <type_traits>
#include <concepts>
#include <cstdint>

#include "HlslTypes.h"
#include "ArenaAllocator.hpp"
#include "Bits.hpp"


struct RenderCommand
{
	uint32_t meshId;
	uint32_t materialId;
	uint32_t psoId;
	uint32_t modelMatId;
	uint32_t boneOffset;
	uint32_t boneCount;
	SizedBitField<uint32_t> state;
};


template <typename T>
struct is_specialization_of_HeapArenaAllocator : std::false_type {};

template <typename U>
struct is_specialization_of_HeapArenaAllocator<HeapArenaAllocator<U>> : std::true_type {};

template<typename A>
concept DerivedFromHeapArenaAllocator = is_specialization_of_HeapArenaAllocator<A>::value;

template <DerivedFromHeapArenaAllocator Alloc>
struct FrameData
{
	std::vector<RenderCommand> renderCommands;
	CameraData globals;
	std::vector<glm::mat4, Alloc<VSInput>> modelMatrices
};

#endif