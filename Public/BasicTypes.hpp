#ifndef BASICTYPES_HPP
#define BASICTYPES_HPP

#include <cstdint>
#include <array>
#include <assimp/texture.h>
#include <span>
#include <limits>

#include "GlobalMacros.h"

struct ColorRGBA_f
{
	~ColorRGBA_f() = default;
	ColorRGBA_f() = default;
	ColorRGBA_f(const ColorRGBA_f& other) = default;
	ColorRGBA_f& operator=(const ColorRGBA_f& other) = default;

	float r{ 0.5f };
	float g{ 0.5f };
	float b{ 0.5f };
	float a{ 0.5f };

	float& operator[] (size_t idx) {
		if (idx > 3) idx = 3;
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
		}
	}

	float operator[] (size_t idx) const {
		if (idx > 3) idx = 3;
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
		}
	}
};




struct ColorRGBA
{
	uint8_t r{ 128 };
	uint8_t g{ 128 };
	uint8_t b{ 128 };
	uint8_t a{ 128 };

	~ColorRGBA() = default;
	ColorRGBA() = default;
	ColorRGBA(const ColorRGBA_f& color_f) {
		r = static_cast<uint8_t>(std::max(color_f.r, 1.0f) * 255);
		g = static_cast<uint8_t>(std::max(color_f.g, 1.0f) * 255);
		b = static_cast<uint8_t>(std::max(color_f.b, 1.0f) * 255);
		a = static_cast<uint8_t>(std::max(color_f.a, 1.0f) * 255);
	}
	ColorRGBA(const aiTexel& texel) {
		r = texel.r;
		g = texel.g;
		b = texel.b;
		a = texel.a;
	}
	ColorRGBA(const ColorRGBA& other) = default;
	ColorRGBA& operator=(const ColorRGBA& other) = default;


	uint8_t& operator[] (size_t idx) {
		if (idx > 3) idx = 3;
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
		}
	}

	uint8_t operator[] (size_t idx) const {
		if (idx > 3) idx = 3;
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
		}
	}

	ColorRGBA_f asColorRGBA_f() {
		ColorRGBA_f arr;
		arr[0] = static_cast<float>(r) / 255.0f;
		arr[1] = static_cast<float>(g) / 255.0f;
		arr[2] = static_cast<float>(b) / 255.0f;
		arr[3] = static_cast<float>(a) / 255.0f;
		return arr;
	}
};


struct FrustumPlane
{
	union
	{
		float raw[4]{};
		struct
		{
			glm::vec3 normal;
			float d;
		};
		struct
		{
			float x, y, z, d;
		};
		glm::vec4 vec;
	};
};

struct Frustum
{
	FrustumPlane planes[6];
	enum { Left, Right, Bottom, Top, Near, Far };
};

enum class BoundingShape : uint8_t
{
	BSphere,
	AABB,
	OBB
};

struct BSphere
{
	glm::vec3 center{ 0,0,0 };
	float radius{ 1.0f };

	static constexpr BoundingShape shape() { return BoundingShape::BSphere; }

	void encapsule(const std::span<BaseVSIn> vertices) {

		float maxDistSqr = std::numeric_limits<float>::min();

		for (auto& vert : vertices) {
			auto delta = vert.pos - center;
			auto thisDistSqr = glm::dot(delta, delta);
			maxDistSqr = std::max(maxDistSqr, thisDistSqr);
		}
		radius = glm::sqrt(maxDistSqr);
	}
};

struct AABB
{
	glm::vec3 frontTopLeft{ -0.5f, 0.5f, 0.5f };
	glm::vec3 backBottomRight{0.5f, -0.5f, -0.5f};
	static constexpr BoundingShape shape() { return BoundingShape::AABB; }
};

struct OBB
{
	glm::vec3 frontTopLeft{ -0.5f, 0.5f, 0.5f };
	glm::vec3 backBottomRight{ 0.5f, -0.5f, -0.5f };
	glm::quat rotation;
	static constexpr BoundingShape shape() { return BoundingShape::OBB; }
};


struct BoundingVolumeComponent
{
	using BoundVolumePair = std::pair<BoundingShape, uint32_t>;

	~BoundingVolumeComponent() = default;
	BoundingVolumeComponent() :
		fineRawData{0}, coarseIndex{ UINT32_INVALID } {}
	BoundingVolumeComponent(const uint32_t coarseIndex) :
		fineRawData{ 0 }, coarseIndex{coarseIndex} {}
	BoundingVolumeComponent(const uint32_t coarseIndex, const BoundingShape firstShape, const uint32_t firstFineIndex) :
		fineRawData{ 0 },
		coarseIndex{ coarseIndex }
	{
		fineCount = 1;
		fineIndex[0] = firstFineIndex;
		fineType0 = static_cast<uint32_t>(firstShape);
		fineEnabled0 = 1;
	}
	BoundingVolumeComponent(const BoundingVolumeComponent& other) {
		fineRawData = other.fineRawData;
		coarseIndex = other.coarseIndex;
		std::memcpy(fineIndex, other.fineIndex, 6 * sizeof(uint32_t));
	}
	BoundingVolumeComponent& operator=(const BoundingVolumeComponent& rhs) {
		if (&rhs == this)
			return *this;

		fineRawData = rhs.fineRawData;
		coarseIndex = rhs.coarseIndex;
		std::memcpy(fineIndex, rhs.fineIndex, 6 * sizeof(uint32_t));

		return *this;
	}
	bool operator==(const BoundingVolumeComponent& rhs) const {
		return
			fineRawData == rhs.fineRawData &&
			std::memcmp(fineIndex, rhs.fineIndex, 6 * sizeof(uint32_t)) == 0 &&
			coarseIndex == rhs.coarseIndex;
	}

	std::vector<BoundingShape> getFineShapes() const {
		std::vector<BoundingShape> shapes;
		uint8_t count = 0;
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back(static_cast<BoundingShape>(fineType0));
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back(static_cast<BoundingShape>(fineType1));
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back(static_cast<BoundingShape>(fineType2));
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back(static_cast<BoundingShape>(fineType3));
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back(static_cast<BoundingShape>(fineType4));
		if (fineCount == count)
			return shapes;
		shapes.push_back(static_cast<BoundingShape>(fineType5));
		
		return shapes;
	}

	std::vector<BoundVolumePair> getBoundingVolumePairs() const {
		std::vector<BoundVolumePair> shapes;
		uint8_t count = 0;
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back( { static_cast<BoundingShape>(fineType0), fineIndex[0] } );
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back({ static_cast<BoundingShape>(fineType1), fineIndex[1] });
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back({ static_cast<BoundingShape>(fineType2), fineIndex[2] });
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back({ static_cast<BoundingShape>(fineType3), fineIndex[3] });
		if (fineCount == count)
			return shapes;
		count++;
		shapes.push_back({ static_cast<BoundingShape>(fineType4), fineIndex[4] });
		if (fineCount == count)
			return shapes;
		shapes.push_back({ static_cast<BoundingShape>(fineType5), fineIndex[5] });

		return shapes;
	}

	void enabledFine(const uint8_t index, bool onOff) {
		switch (index) {
			default: return;
			case 0: if (fineCount >= 1) fineEnabled0 = onOff ? 1 : 0; return;
			case 1: if (fineCount >= 2) fineEnabled1 = onOff ? 1 : 0; return;
			case 2: if (fineCount >= 3) fineEnabled2 = onOff ? 1 : 0; return;
			case 3: if (fineCount >= 4) fineEnabled3 = onOff ? 1 : 0; return;
			case 4: if (fineCount >= 5) fineEnabled4 = onOff ? 1 : 0; return;
			case 5: if (fineCount >= 6) fineEnabled5 = onOff ? 1 : 0; return;
		}
	}

	bool enabled(const uint8_t index) const {
		switch (index) {
			default: return false;
			case 0: return (fineEnabled0 ==  1 ? true : false) && fineCount >= 1;
			case 1: return (fineEnabled1 ==  1 ? true : false) && fineCount >= 2;
			case 2: return (fineEnabled2 ==  1 ? true : false) && fineCount >= 3;
			case 3: return (fineEnabled3 ==  1 ? true : false) && fineCount >= 4;
			case 4: return (fineEnabled4 ==  1 ? true : false) && fineCount >= 5;
			case 5: return (fineEnabled5 ==  1 ? true : false) && fineCount >= 6;
		}
	}

	bool addFine(const BoundingShape& shape, uint32_t index) {
		auto lastCount = fineCount;
		fineCount = std::min(fineCount + 1, (uint32_t)6);
		switch (fineCount) {
			default: fineCount = lastCount; return false;

			case 1:
			{
				fineType0 = static_cast<uint32_t>(shape);
				fineEnabled0 = 1;
			} break;
			case 2:
			{
				fineType1 = static_cast<uint32_t>(shape);
				fineEnabled1= 1;
			} break;
			case 3:
			{
				fineType2 = static_cast<uint32_t>(shape);
				fineEnabled2 = 1;
			} break;
			case 4:
			{
				fineType3 = static_cast<uint32_t>(shape);
				fineEnabled3 = 1;
			} break;
			case 5:
			{
				fineType4 = static_cast<uint32_t>(shape);
				fineEnabled4 = 1;
			} break;
			case 6:
			{
				fineType5 = static_cast<uint32_t>(shape);
				fineEnabled5 = 1;
			} break;
		}
		fineIndex[fineCount - 1] = index;
		return true;
	}

	union
	{
		uint32_t fineRawData;
		struct
		{
			uint32_t fineCount		: 4;
			uint32_t fineType0		: 2;
			uint32_t fineType1		: 2;
			uint32_t fineType2		: 2;
			uint32_t fineType3		: 2;
			uint32_t fineType4		: 2;
			uint32_t fineType5		: 2;
			uint32_t fineEnabled0	: 1;
			uint32_t fineEnabled1	: 1;
			uint32_t fineEnabled2	: 1;
			uint32_t fineEnabled3	: 1;
			uint32_t fineEnabled4	: 1;
			uint32_t fineEnabled5	: 1;
		};
	};
	uint32_t fineIndex[6]{
		UINT32_INVALID, UINT32_INVALID, UINT32_INVALID, UINT32_INVALID, UINT32_INVALID, UINT32_INVALID,
	};
	uint32_t coarseIndex; // <- this is always a BSphere
	// The whole struct fits in 32bytes, and most of the time
	// 'fineCount' can be 0 and therefore no more checks will
	// be made, but the datatype allows for easy extension
	// without introducing more buffers and vectors.
};

#endif