#ifndef RANGE_HPP
#define RANGE_HPP

#include <cstdint>
#include "GlobalMacros.h"

template <std::integral T>
struct Range
{
	T offset;
	T count;
	~Range() = default;
	Range() :
		offset{0}, count{0} {}

	template <std::integral U>
	Range(U offset, U count) :
		offset{ static_cast<T>(offset) }, count{ static_cast<T>(count) } {}

	template <std::integral U, std::integral V>
	Range(U offset, V count) :
		offset{ static_cast<T>(offset) }, count{ static_cast<T>(count) } {}

	template <std::integral U>
	Range(const Range<U>& other) :
		offset{ static_cast<T>(offset) }, count{ static_cast<T>(count) } {}

	template <std::integral U>
	Range& operator=(const Range<U>& rhs) {
		if (this == &rhs)
			return *this;
		offset = static_cast<T>(rhs.offset);
		count = static_cast<T>(rhs.count);
		return *this;
	}

	template <std::integral U>
	bool operator==(const Range<U>& rhs) {
		return offset == static_cast<T>(rhs.offset) && count == static_cast<T>(rhs.count);
	}

	template <std::integral U>
	T operator[](U idx) {
		if (static_cast<T>(idx) >= count)
			throw std::out_of_range("Out of range of... range...");
		return offset + static_cast<T>(idx);
	}

	INLINE size_t end() { return static_cast<size_t>(offset + count); }
	INLINE size_t start() { return static_cast<size_t>(offset); }
};

#endif