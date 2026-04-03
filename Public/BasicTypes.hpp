#ifndef BASICTYPES_HPP
#define BASICTYPES_HPP

#include <cstdint>
#include <array>
#include <assimp/texture.h>
#include <span>
#include <limits>


#include "GlobalMacros.h"
#include "MMath.hpp"
#include "glm/gtc/type_ptr.hpp"


struct ColorRGBA_f
{
	~ColorRGBA_f() = default;
	ColorRGBA_f() = default;
	explicit ColorRGBA_f(const float all, const float A = 1.0f) {
		r = all;
		g = all;
		b = all;
		a = A;
	}
	ColorRGBA_f(const float R, const float G, const float B, const float A = 1.0f) {
		r = R;
		b = B;
		g = G;
		a = A;	
	}
		
	ColorRGBA_f(const ColorRGBA_f& other) = default;
	ColorRGBA_f& operator=(const ColorRGBA_f& other) = default;

	union
	{
		float raw[4]{ 0.5f, 0.5f, 0.5f, 1.0f };
		struct
		{
			float r;
			float g;
			float b;
			float a;
		};
	};

	explicit operator glm::vec4() const {
		return {r, g, b, a};
	}


	float& operator[] (const size_t idx) {
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			default:
				return a;
		}
	}

	float operator[] (const size_t idx) const {
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			default:
				return a;
		}
	}
};

template <std::integral T>
INLINE ColorRGBA_f colorCycle(const T step, const float a) {

	switch (const auto s = step % 18; s) {

		case 0:  return	ColorRGBA_f{ 1.0f,		0.0f,		0.0f,		a };
		case 1: return	ColorRGBA_f{ 0.951185f,	0.117851f,	0.0f,		a };
		case 2: return	ColorRGBA_f{ 0.853554f,	0.353554f,	0.0f,		a };
		case 3: return	ColorRGBA_f{ 0.707107f,	0.707107f,	0.0f,		a };
		case 4: return	ColorRGBA_f{ 0.353554f,	0.853554f,	0.0f,		a };
		case 5: return	ColorRGBA_f{ 0.117851f,	0.951185f,	0.0f,		a };

		case 6: return	ColorRGBA_f{ 0.0f,		1.0f,		0.0f,		a };
		case 7: return	ColorRGBA_f{ 1.0f,		0.951185f,	0.117851f,	a };
		case 8: return	ColorRGBA_f{ 1.0f,		0.853554f,	0.353554f,	a };
		case 9: return	ColorRGBA_f{ 1.0f,		0.707107f,	0.707107f,	a };
		case 10: return	ColorRGBA_f{ 0.0f,		0.353554f,	0.853554f,	a };
		case 11: return	ColorRGBA_f{ 0.0f,		0.117851f,	0.951185f,	a };

		case 12: return	ColorRGBA_f{ 0.0f,		0.0f,		1.0f,		a };
		case 13: return	ColorRGBA_f{ 0.0f,		0.0f,		0.951185f,	a };
		case 14: return	ColorRGBA_f{ 0.0f,		0.0f,		0.853554f,	a };
		case 15: return	ColorRGBA_f{ 0.707107f,	0.0f,		0.707107f,	a };
		case 16: return	ColorRGBA_f{ 0.853554f,	0.0f,		0.353554f,	a };
		default: return	ColorRGBA_f{ 0.951185f,	0.0f,		0.117851f,	a };
	}
}


struct ColorRGBA
{
	uint8_t r{ 128 };
	uint8_t g{ 128 };
	uint8_t b{ 128 };
	uint8_t a{ 128 };

	~ColorRGBA() = default;
	ColorRGBA() = default;
	explicit ColorRGBA(const ColorRGBA_f& color_f) {
		r = static_cast<uint8_t>(std::max(color_f.r, 1.0f) * 255);
		g = static_cast<uint8_t>(std::max(color_f.g, 1.0f) * 255);
		b = static_cast<uint8_t>(std::max(color_f.b, 1.0f) * 255);
		a = static_cast<uint8_t>(std::max(color_f.a, 1.0f) * 255);
	}
	explicit ColorRGBA(const aiTexel& texel) {
		r = texel.r;
		g = texel.g;
		b = texel.b;
		a = texel.a;
	}
	ColorRGBA(const ColorRGBA& other) = default;
	ColorRGBA& operator=(const ColorRGBA& other) = default;


	uint8_t& operator[] (const size_t idx) {
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			default:
				return a;
		}
	}

	uint8_t operator[] (const size_t idx) const {
		switch (idx) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			default:
				return a;
		}
	}

	[[nodiscard]] ColorRGBA_f asColorRGBA_f() const {
		ColorRGBA_f arr;
		arr[0] = static_cast<float>(r) / 255.0f;
		arr[1] = static_cast<float>(g) / 255.0f;
		arr[2] = static_cast<float>(b) / 255.0f;
		arr[3] = static_cast<float>(a) / 255.0f;
		return arr;
	}
};

struct ZRow
{
	union
	{
		float raw[4];
		struct
		{
			float n[3];
			float w;
		};
	};

	[[nodiscard]] glm::vec3 getN() const {
		return glm::make_vec3(n);
	}
};

struct Ray
{
	~Ray() = default;
	Ray() :
		origin{0,0,0},
		direction{0,0,-1},
		invDirection{ 1.0f / direction } {}
	Ray(const glm::vec3& start, const glm::vec3& direction) :
		origin{start},
		direction{glm::normalize(direction)},
		invDirection{1.0f / direction} {}

	Ray(const Ray& other) = default;
	Ray& operator=(const Ray& rhs) = default;

	glm::vec3 origin;
	glm::vec3 direction;
	glm::vec3 invDirection;
	
	bool operator==(const Ray& rhs) const {
		return origin == rhs.origin && direction == rhs.direction;
	}

	bool operator!=(const Ray& rhs) const {
		return !(*this == rhs);
	}
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

	void encloseLocal(const std::span<BaseVSIn> vertices) {

		float maxDistSqr = std::numeric_limits<float>::min();

		for (auto& vert : vertices) {
			auto delta = vert.pos - center;
			auto thisDistSqr = glm::dot(delta, delta);
			maxDistSqr = std::max(maxDistSqr, thisDistSqr);
		}
		radius = glm::sqrt(maxDistSqr);
	}
};



struct alignas(16) AABB
{
private:
	INLINE void _getVerticesSoA(__m256& X, __m256& Y, __m256& Z, __m256& W) const noexcept {
		// const float xs[8] = {
		// 	min[0],
		// 	max[0],
		// 	min[0],
		// 	max[0],
		// 	min[0],
		// 	max[0],
		// 	min[0],
		// 	max[0]
		// };
		// const float ys[8] = {
		// 	max[1],
		// 	max[1],
		// 	min[1],
		// 	min[1],
		// 	max[1],
		// 	max[1],
		// 	min[1],
		// 	min[1]
		// };
		// const float zs[8] = {
		// 	max[2],
		// 	max[2],
		// 	max[2],
		// 	max[2],
		// 	min[2],
		// 	min[2],
		// 	min[2],
		// 	min[2]
		// };
		// X = _mm256_loadu_ps(xs);
		// Y = _mm256_loadu_ps(ys);
		// Z = _mm256_loadu_ps(zs);
		const float minx = min.x, maxx = max.x;
		const float miny = min.y, maxy = max.y;
		const float minz = min.z, maxz = max.z;

		X = _mm256_setr_ps(minx, maxx, minx, maxx, minx, maxx, minx, maxx);
		Y = _mm256_setr_ps(maxy, maxy, miny, miny, maxy, maxy, miny, miny);
		Z = _mm256_setr_ps(maxz, maxz, maxz, maxz, minz, minz, minz, minz);
		W = _mm256_set1_ps(1.0f);
	}

public:
	AABB() noexcept = default;

	AABB(const glm::vec3& minVec, const glm::vec3& maxVec)
	{
		min = minVec;
		max = maxVec;

		//MMath::toFloat<3,3>(minVec3, minVec);
		//MMath::toFloat<3,3>(maxVec3, maxVec);
	}
	// union
	// {
	// 	float raw[8]{ -0.5f, -0.5f, -0.5f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f };
	// 	struct
	// 	{
	// 		float minVec3[3];
	// 		float _pad0;
	// 		float maxVec3[3];
	// 		float _pad1;
	// 	};
	// 	struct
	// 	{
	// 		float minVec4[4], maxVec4[4];
	// 	};
	// 	struct
	// 	{
	// 		float minX, minY, minZ, minW;
	// 		float maxX, maxY, maxZ, maxW;
	// 	};
	// };
	glm::vec3 min{-0.5f, -0.5f, -0.5f};
	glm::vec3 max{0.5f, 0.5f, 0.5f};

	// [[nodiscard]] INLINE glm::vec3 min() const {
	// 	return glm::make_vec3(minVec3);
	// }
	//
	// [[nodiscard]] INLINE glm::vec3 max() const {
	// 	return glm::make_vec3(maxVec3);
	// }
	//
	// [[nodiscard]] INLINE glm::vec4 minPoint() const {
	// 	return glm::make_vec4(minVec4);
	// }
	// [[nodiscard]] INLINE glm::vec4 maxPoint() const {
	// 	return glm::make_vec4(maxVec4);
	// }

	// INLINE void setMin(const glm::vec3& minVec) {
	// 	minX = minVec.x;
	// 	minY = minVec.y;
	// 	minZ = minVec.z;
	// }
	// INLINE void setMax(const glm::vec3& maxVec) {
	// 	maxX = maxVec.x;
	// 	maxY = maxVec.y;
	// 	maxZ = maxVec.z;
	// }

	[[nodiscard]] INLINE std::array<glm::vec3, 8> getVertices() const {

		std::array<glm::vec3, 8> verts; // NOLINT(*-pro-type-member-init)
		verts[0] = glm::vec3{ min.x, max.y, max.z };
		verts[1] = glm::vec3{ max.x, max.y, max.z };
		verts[2] = glm::vec3{ min.x, min.y, max.z };
		verts[3] = glm::vec3{ max.x, min.y, max.z };
		verts[4] = glm::vec3{ min.x, max.y, min.z };
		verts[5] = glm::vec3{ max.x, max.y, min.z };
		verts[6] = glm::vec3{ min.x, min.y, min.z };
		verts[7] = glm::vec3{ max.x, min.y, min.z };

		return verts;
	}

	[[nodiscard]] INLINE AABB transformed_noPerspective(const glm::mat4x4& trs) const {

		const glm::vec3 c = 0.5f * (min + max);
		const glm::vec3 e = 0.5f * (max - min);
		const auto A = glm::mat3(trs);
		const auto nc = glm::vec3(trs * glm::vec4(c, 1.0f));

		const auto B = glm::mat3(glm::abs(A[0]), glm::abs(A[1]), glm::abs(A[2]));
		const glm::vec3 ne = B * e;
		return {nc - ne, nc + ne};
	}

	INLINE AABB& transform_noPerspective(const glm::mat4x4& trs) {
		const glm::vec3 c = 0.5f * (min + max);
		const glm::vec3 e = 0.5f * (max - min);
		const auto A = glm::mat3(trs);
		const auto nc = glm::vec3(trs * glm::vec4(c, 1.0f));

		const auto B = glm::mat3(glm::abs(A[0]), glm::abs(A[1]), glm::abs(A[2]));
		const glm::vec3 ne = B * e;

		min = nc - ne;
		max = nc + ne;

		return *this;
	}

	[[nodiscard]] INLINE static AABB transformed_noPerspective(const AABB& box, const glm::mat4x4& trs) {
		const glm::vec3 c = 0.5f * (box.min + box.max);
		const glm::vec3 e = 0.5f * (box.max - box.min);
		const auto A = glm::mat3(trs);
		const auto nc = glm::vec3(trs * glm::vec4(c, 1.0f));

		const auto B = glm::mat3(glm::abs(A[0]), glm::abs(A[1]), glm::abs(A[2]));
		const glm::vec3 ne = B * e;
		return {nc - ne, nc + ne};
	}

	[[nodiscard]] INLINE AABB transformed_perspective(const glm::mat4x4& M) const noexcept {
		
		__m256 X, Y, Z, W; _getVerticesSoA(X, Y, Z, W);
		const float m00 = M[0][0], m01 = M[1][0], m02 = M[2][0], m03 = M[3][0];
		const float m10 = M[0][1], m11 = M[1][1], m12 = M[2][1], m13 = M[3][1];
		const float m20 = M[0][2], m21 = M[1][2], m22 = M[2][2], m23 = M[3][2];
		const float m30 = M[0][3], m31 = M[1][3], m32 = M[2][3], m33 = M[3][3];

		const __m256 r0x = _mm256_set1_ps(m00), r0y = _mm256_set1_ps(m01), r0z = _mm256_set1_ps(m02), r0w = _mm256_set1_ps(m03);
		const __m256 r1x = _mm256_set1_ps(m10), r1y = _mm256_set1_ps(m11), r1z = _mm256_set1_ps(m12), r1w = _mm256_set1_ps(m13);
		const __m256 r2x = _mm256_set1_ps(m20), r2y = _mm256_set1_ps(m21), r2z = _mm256_set1_ps(m22), r2w = _mm256_set1_ps(m23);
		const __m256 r3x = _mm256_set1_ps(m30), r3y = _mm256_set1_ps(m31), r3z = _mm256_set1_ps(m32), r3w = _mm256_set1_ps(m33);

// //#if defined(__FMA__) || (defined(_MSC_VER) && defined(__AVX2__))
// 		auto fmadd = [](const __m256 a, const __m256 b, const __m256 c) { return _mm256_fmadd_ps(a, b, c); };
// // #else
// // 		auto fmadd = [](const __m256 a, const __m256 b, const __m256 c) {
// // 			return _mm256_add_ps(_mm256_mul_ps(a, b), c);
// // 		};
// // #endif


//NOLINTBEGIN(portability-simd-intrinsics)
#if defined(__FMA__)
#define fmadd(a,b,c) _mm256_fmadd_ps((a),(b),(c))
#else
#define fmadd(a,b,c) _mm256_add_ps(_mm256_mul_ps((a),(b)), (c))
#endif

		const __m256 Xp = fmadd(r0x, X, fmadd(r0y, Y, fmadd(r0z, Z, _mm256_mul_ps(r0w, W))));
		const __m256 Yp = fmadd(r1x, X, fmadd(r1y, Y, fmadd(r1z, Z, _mm256_mul_ps(r1w, W))));
		const __m256 Zp = fmadd(r2x, X, fmadd(r2y, Y, fmadd(r2z, Z, _mm256_mul_ps(r2w, W))));
		const __m256 Wp = fmadd(r3x, X, fmadd(r3y, Y, fmadd(r3z, Z, _mm256_mul_ps(r3w, W))));

		// Safe reciprocal of W (preserves sign, clamps |W| >= eps)
		const __m256 eps = _mm256_set1_ps(1e-20f);
		const __m256 signmsk = _mm256_set1_ps(-0.0f);                 // 0x80000000
		const __m256 sign = _mm256_and_ps(Wp, signmsk);            // keep sign of W
		const __m256 absv = _mm256_andnot_ps(signmsk, Wp);         // |W|
		const __m256 mag = _mm256_max_ps(absv, eps);              // clamp
		const __m256 Wsafe = _mm256_or_ps(sign, mag);               // restore sign
		const __m256 invW = _mm256_div_ps(_mm256_set1_ps(1.0f), Wsafe);

		const __m256 Xn = _mm256_mul_ps(Xp, invW);
		const __m256 Yn = _mm256_mul_ps(Yp, invW);
		const __m256 Zn = _mm256_mul_ps(Zp, invW);

		float xmin, xmax, ymin, ymax, zmin, zmax;
		MMath::hminmax8(Xn, xmin, xmax);
		MMath::hminmax8(Yn, ymin, ymax);
		MMath::hminmax8(Zn, zmin, zmax);

		return {glm::vec3{ xmin, ymin, zmin }, glm::vec3{ xmax, ymax, zmax }};
//NOLINTEND(portability-simd-intrinsics)
	}

	INLINE AABB& transform_perspective(const glm::mat4x4& M) {
		const auto newAABB = transformed_perspective(M);

		min.x = newAABB.min.x;
		min.y = newAABB.min.y;
		min.z = newAABB.min.z;
		max.x = newAABB.max.x;
		max.y = newAABB.max.y;
		max.z = newAABB.max.z;

		return *this;
	}

	static constexpr BoundingShape shape() { return BoundingShape::AABB; }

	INLINE void encloseLocal(const std::span<BaseVSIn> vertices) {

		if (vertices.empty()) return;

		glm::vec3 vmin(std::numeric_limits<float>::max());
		glm::vec3 vmax(-std::numeric_limits<float>::max());

		for (const auto& v : vertices) {
			vmin = glm::min(vmin, v.pos);
			vmax = glm::max(vmax, v.pos);
		}

		min = vmin;
		max = vmax;
	}

	[[nodiscard]] INLINE bool intersects(const AABB& box) const {
		return
			min.x <= box.max.x &&
			max.x >= box.min.x &&
			min.y <= box.max.y &&
			max.y >= box.min.y &&
			min.z <= box.max.z &&
			max.z >= box.min.z;
	}

	[[nodiscard]] INLINE bool intersects(const Ray& r) const {
		double tmin = -HUGE_VAL, tmax = HUGE_VAL;

		for (int i = 0; i < 3; ++i) {
			double t1 = (min[i] - r.origin[i]) * r.invDirection[i];
			double t2 = (max[i] - r.origin[i]) * r.invDirection[i];

			tmin = std::max(tmin, std::min(t1, t2));
			tmax = std::min(tmax, std::max(t1, t2));
		}

		return tmax > std::max(tmin, 0.0);
	}

	[[nodiscard]] INLINE bool contains(const glm::vec3& point) const {
		return
			point.x >= min.x && point.x <= max.x &&
			point.y >= min.y && point.y <= max.y &&
			point.z >= min.z && point.z <= max.z;
	}

	[[nodiscard]] INLINE bool contains(const AABB& box) const {
		return
			box.min.x >= min.x && box.max.x <= max.x &&
			box.min.y >= min.y && box.max.y <= max.y &&
			box.min.z >= min.z && box.max.z <= max.z;
	}

	
	INLINE AABB& merge(const AABB& other) {
		min = glm::min(min, other.min);
		max = glm::max(max, other.max);
		return *this;
	}

	static AABB merge(const AABB& a, const AABB& b) {
		const AABB c{ glm::min(a.min, b.min) , glm::max(a.max, b.max) };
		return c;
	}

	INLINE void encloseWorld(const AABB& local, const glm::mat4x4& trs) {

		const glm::vec3 c = 0.5f * (local.min + local.max);
		const glm::vec3 e = 0.5f * (local.max - local.min);

		const auto wc = glm::vec3(trs * glm::vec4(c, 1.0f));
		const auto M = glm::mat3(trs);
		auto A = glm::mat3(
			glm::abs(M[0]),
			glm::abs(M[1]),
			glm::abs(M[2])
		);
		const glm::vec3 we = A * e;

		const glm::vec4 wmin = {wc - we, 1.0f};
		const glm::vec4 wmax = {wc + we, 1.0f};

		min = wmin;
		max = wmax;
	}
	INLINE void encloseWorld_fromLocal(const glm::mat4x4& trs) { encloseWorld(*this, trs); }

	[[nodiscard]] INLINE float width() const {
		return max.x - min.x;
	}
	[[nodiscard]] INLINE float height() const {
		return max.y - min.y;
	}
	[[nodiscard]] INLINE float depth() const {
		return max.z - min.z;
	}
	[[nodiscard]] INLINE float volume() const {
		return width() * height() * depth();
	}
	[[nodiscard]] INLINE glm::vec3 center() const {
		return {
			min.x + (max.x - min.x) * 0.5f,
			min.y + (max.y - min.y) * 0.5f,
			min.z + (max.z - min.z) * 0.5f
			};
	}
	[[nodiscard]] INLINE glm::vec3 size() const {
		return {
			max.x - min.x,
			max.y - min.y,
			max.z - min.z
		};
	}
	[[nodiscard]] INLINE glm::vec3 extent() const {
		return max - min;
	}
	[[nodiscard]] INLINE float surfaceArea() const {
		const glm::vec3 extent = max - min;
		return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
	}

	INLINE void* data() noexcept {
		return &min;
	}
	[[nodiscard]] INLINE const void* data() const noexcept {
		return &min;
	}

	static constexpr size_t byteSize() noexcept { return sizeof(AABB); }

	bool operator==(const AABB& rhs) const {
		return min == max && rhs.min == max;
	}

	bool operator!=(const AABB& rhs) const {
		return !(*this == rhs);
	}
};

struct OBB
{
	glm::vec3 frontTopLeft{-0.5f, 0.5f, 0.5f};
	glm::vec3 backBottomRight{0.5f, -0.5f, -0.5f };
	glm::quat rotation;
	static constexpr BoundingShape shape() { return BoundingShape::OBB; }
};

enum class BoundingVolumeFlags : uint8_t
{
	None = 0x00,
	Static = 0x01,
	Occluder = 0x02,
	Occludee = 0x04,
	ShadowCast = 0x08
};

struct BoundingVolumeComponent
{
	using BoundVolumePair = std::pair<BoundingShape, uint32_t>;

	~BoundingVolumeComponent() = default;
	BoundingVolumeComponent() : // NOLINT(*-pro-type-member-init)
		rawData{ 0 },
		coarseIndexLocal{ UINT32_INVALID },
		coarseIndexWorld{ UINT32_INVALID }
	{
		flags = static_cast<uint32_t>(BoundingVolumeFlags::Occludee);
	}

	BoundingVolumeComponent(const uint32_t coarseIndexLocal, const uint32_t coarseIndexWorld) : // NOLINT(*-pro-type-member-init)
		rawData{ 0 },
		coarseIndexLocal{coarseIndexLocal},
		coarseIndexWorld{ coarseIndexWorld }
	{
		flags = static_cast<uint32_t>(BoundingVolumeFlags::Occludee);
	}

	//BoundingVolumeComponent(const uint32_t coarseIndex, const BoundingShape firstShape, const uint32_t firstFineIndex) :
	//	rawData{ 0 },
	//	coarseIndexLocal{ coarseIndex }
	//{
	//	fineCount = 1;
	//	fineIndex[0] = firstFineIndex;
	//	fineType0 = static_cast<uint32_t>(firstShape);
	//	fineEnabled0 = 1;
	//}
	BoundingVolumeComponent(const BoundingVolumeComponent& other) { // NOLINT(*-pro-type-member-init)
		rawData = other.rawData;
		coarseIndexLocal = other.coarseIndexLocal;
		coarseIndexWorld = other.coarseIndexWorld;
		std::memcpy(fineIndex, other.fineIndex, 4 * sizeof(uint32_t));
	}
	BoundingVolumeComponent& operator=(const BoundingVolumeComponent& rhs) {
		if (&rhs == this)
			return *this;

		rawData = rhs.rawData;
		coarseIndexLocal = rhs.coarseIndexLocal;
		coarseIndexWorld = rhs.coarseIndexWorld;
		std::memcpy(fineIndex, rhs.fineIndex, 4 * sizeof(uint32_t));

		return *this;
	}
	bool operator==(const BoundingVolumeComponent& rhs) const {
		return
			rawData == rhs.rawData &&
			std::memcmp(fineIndex, rhs.fineIndex, 4 * sizeof(uint32_t)) == 0 &&
			coarseIndexLocal == rhs.coarseIndexLocal &&
			coarseIndexWorld == rhs.coarseIndexWorld;
	}

	[[nodiscard]] std::vector<BoundingShape> getFineShapes() const {
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
		//count++;
		//shapes.push_back(static_cast<BoundingShape>(fineType4));
		//if (fineCount == count)
		//	return shapes;
		//shapes.push_back(static_cast<BoundingShape>(fineType5));
		
		return shapes;
	}

	[[nodiscard]] std::vector<BoundVolumePair> getBoundingVolumePairs() const {
		std::vector<BoundVolumePair> shapes;
		uint8_t count = 0;
		if (fineCount == count)
			return shapes;
		count++;
		shapes.emplace_back( static_cast<BoundingShape>(fineType0), fineIndex[0] );
		if (fineCount == count)
			return shapes;
		count++;
		shapes.emplace_back(static_cast<BoundingShape>(fineType1), fineIndex[1]);
		if (fineCount == count)
			return shapes;
		count++;
		shapes.emplace_back(static_cast<BoundingShape>(fineType2), fineIndex[2]);
		if (fineCount == count)
			return shapes;
		count++;
		shapes.emplace_back(static_cast<BoundingShape>(fineType3), fineIndex[3]);
		if (fineCount == count)
			return shapes;
		//count++;
		//shapes.push_back({ static_cast<BoundingShape>(fineType4), fineIndex[4] });
		//if (fineCount == count)
		//	return shapes;
		//shapes.push_back({ static_cast<BoundingShape>(fineType5), fineIndex[5] });

		return shapes;
	}

	void enabledFine(const uint8_t index, const bool onOff) {
		switch (index) {
			default: return;
			case 0: if (fineCount >= 1) fineEnabled0 = onOff ? 1 : 0; return;
			case 1: if (fineCount >= 2) fineEnabled1 = onOff ? 1 : 0; return;
			case 2: if (fineCount >= 3) fineEnabled2 = onOff ? 1 : 0; return;
			case 3: if (fineCount >= 4) fineEnabled3 = onOff ? 1 : 0;
			// return; case 4: if (fineCount >= 5) fineEnabled4 = onOff ? 1 : 0; return;
			//case 5: if (fineCount >= 6) fineEnabled5 = onOff ? 1 : 0; return;
		}
	}

	[[nodiscard]] bool enabled(const uint8_t index) const {
		switch (index) {
			default: return false;
			case 0: return fineEnabled0 ==  1 && fineCount >= 1;
			case 1: return fineEnabled1 ==  1 && fineCount >= 2;
			case 2: return fineEnabled2 ==  1 && fineCount >= 3;
			case 3: return fineEnabled3 ==  1 && fineCount >= 4;
			//case 4: return (fineEnabled4 ==  1 ? true : false) && fineCount >= 5;
			//case 5: return (fineEnabled5 ==  1 ? true : false) && fineCount >= 6;
		}
	}

	bool addFine(const BoundingShape& shape, const uint32_t index) {
		const auto lastCount = fineCount;
		fineCount = std::min(fineCount + 1u, static_cast<uint32_t>(4)); // max here
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
			//case 5:
			//{
			//	fineType4 = static_cast<uint32_t>(shape);
			//	fineEnabled4 = 1;
			//} break;
			//case 6:
			//{
			//	fineType5 = static_cast<uint32_t>(shape);
			//	fineEnabled5 = 1;
			//} break;
		}
		fineIndex[fineCount - 1] = index;
		return true;
	}

	union
	{
		uint64_t rawData;
		struct
		{
			uint32_t fineCount		: 4;
			uint32_t fineType0		: 2;
			uint32_t fineType1		: 2;
			uint32_t fineType2		: 2;
			uint32_t fineType3		: 2;
			//uint32_t fineType4		: 2;
			//uint32_t fineType5		: 2;
			uint32_t fineEnabled0	: 1;
			uint32_t fineEnabled1	: 1;
			uint32_t fineEnabled2	: 1;
			uint32_t fineEnabled3	: 1;
			//uint32_t fineEnabled4	: 1;
			//uint32_t fineEnabled5	: 1;
			uint32_t _pad0			: 8;
			uint32_t flags			: 8;
			uint32_t layermask		: 32;
		};
	};
	uint32_t fineIndex[4]{
		UINT32_INVALID, UINT32_INVALID, UINT32_INVALID, UINT32_INVALID, //UINT32_INVALID, UINT32_INVALID,
	};
	uint32_t coarseIndexLocal;
	uint32_t coarseIndexWorld;
};



INLINE void aabbViewZRange(const AABB& b, const glm::vec3& n, const float& w,
						   float& zmin, float& zmax)
{
	const glm::vec3 c = 0.5f * (b.min + b.max);          // center
	const glm::vec3 e = 0.5f * (b.max - b.min);          // half-extent

	const float zc = glm::dot(n, c) + w;       // center z
	const glm::vec3 an = glm::abs(n);               // per-axis abs
	const float ze = glm::dot(an, e);                    // projected extent

	zmin = zc - ze;
	zmax = zc + ze;
}

static constexpr uint32_t BVH_MAX_DEPTH = 32;
static constexpr uint32_t BVH_MAX_LEAF_PRIMITIVES = 4;
static constexpr float BVH_REBUILD_THRESHOLD = 0.3f; // 30% of objects moved

struct BuildSettings
{
	uint32_t maxDepth = BVH_MAX_DEPTH;
	uint32_t maxLeafPrimitives = BVH_MAX_LEAF_PRIMITIVES;
	bool useMedianSplit = true;  // false = SAH, true = median
	float rebuildThreshold = BVH_REBUILD_THRESHOLD;
	explicit BuildSettings() {}
};

struct TraversalResult
{
	std::vector<entt::entity> visibleEntities;
	std::vector<entt::entity> activeOccluders;
	std::vector<std::pair<entt::entity, entt::entity>> collisionPairs;
	uint32_t nodesVisited = 0;
	uint32_t primitivesVisited = 0;
	uint32_t nodesCulledByFrustum = 0;
	uint32_t nodesCulledByOcclusion = 0;

	void clear() {
		visibleEntities.clear();
		activeOccluders.clear();
		collisionPairs.clear();
		nodesVisited = 0;
		primitivesVisited = 0;
		nodesCulledByFrustum = 0;
		nodesCulledByOcclusion = 0;
	}
};



#endif