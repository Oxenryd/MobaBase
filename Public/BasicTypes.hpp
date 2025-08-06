#ifndef BASICTYPES_HPP
#define BASICTYPES_HPP

#include <cstdint>
#include <array>
#include <assimp/texture.h>

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

#endif