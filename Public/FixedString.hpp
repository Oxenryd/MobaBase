#ifndef FIXED_STRING_HPP
#define FIXED_STRING_HPP

#include <type_traits>
#include <cstdint>
#include <concepts>
#include <string>
#include <limits>
#include <cstring>
#include <algorithm>

#ifndef FIXED_STRINGS_MAX_LENGTH
	#define FIXED_STRINGS_MAX_LENGTH 62
#endif

#if FIXED_STRINGS_MAX_LENGTH <= 0xfe
	#define FixedString FixedStringBase<static_cast<uint8_t>(FIXED_STRINGS_MAX_LENGTH)>
#elif FIXED_STRINGS_MAX_LENGTH <= 0xfffe
	#define FixedString FixedStringBase<static_cast<uint16_t>(FIXED_STRINGS_MAX_LENGTH)>
#elif FIXED_STRINGS_MAX_LENGTH <= 0xfffffffe
	#define FixedString FixedStringBase<static_cast<uint32_t>(FIXED_STRINGS_MAX_LENGTH)>
#endif


template<auto N>
concept UIntMax254MultipleOf4 =
std::is_integral_v<decltype(N)>
&& !std::is_signed_v<decltype(N)>
&& (N < std::numeric_limits<decltype(N)>::max());


template <auto T>
	requires UIntMax254MultipleOf4<T>
class FixedStringBase
{
	using size_type = decltype(T);
private:
	union
	{
		char _raw[T + 2 * sizeof(size_type)];
		struct
		{
			size_type m_capacity;
			size_type m_size;
			char m_data[T];
		};
	};

	static bool _equalData(const void* first, const void* second, size_t size) {
		return std::memcmp(first, second, size) == 0;
	}


public:
	~FixedStringBase() = default;
	constexpr FixedStringBase() :
		m_size{ 0 }, m_capacity{ T }
	{}
	constexpr explicit FixedStringBase(const std::string& string) :
		m_capacity{ T },
		m_size{ static_cast<size_type>(std::min(static_cast<size_t>(m_capacity), string.size())) }
	{
		std::memcpy(m_data, string.data(), m_size);
	}
	constexpr explicit FixedStringBase(const char* ptr) : 
		m_capacity{T}
	{
		m_size = static_cast<size_type>(std::min(static_cast<size_t>(m_capacity), std::strlen(ptr)));
		std::memcpy(m_data, ptr, m_size);	
	}
	template <auto U>
		requires UIntMax254MultipleOf4<U>
	constexpr explicit FixedStringBase(const FixedStringBase<U>& other) : 
		m_capacity{ T }
	{
		m_size = static_cast<size_type>(std::min(static_cast<size_t>(m_capacity), other.size()));
		std::memcpy(m_data, other.m_data, m_size);
	}

	template <auto U>
		requires UIntMax254MultipleOf4<U>
	FixedStringBase& operator=(const FixedStringBase<U>& rhs) {
		if (this == &rhs)
			return *this;
		m_capacity = T;
		m_size = static_cast<size_type>(std::min(static_cast<size_t>(m_capacity), rhs.size()));
		std::memcpy(m_data, rhs.m_data, m_size);
		
		return *this;
	}
	FixedStringBase& operator=(const std::string& rhs) {
		m_capacity = T;
		m_size = static_cast<size_type>(std::min(static_cast<size_t>(m_capacity), rhs.size()));
		std::memcpy(m_data, rhs.data(), m_size);
		
		return *this;
	}

	FixedStringBase& operator=(const char* ptr) {
		m_capacity = T;
		m_size = static_cast<size_type>(static_cast<size_t>(m_capacity), std::strlen(ptr));
		std::memcpy(m_data, ptr, m_size);		
		return *this;
	}

	constexpr size_t capacity() const { return m_capacity; }
	size_t size() const { return m_size; }
	bool empty() const { return m_size == 0; }
	void clear() { m_size = 0; }
	std::string_view to_stringView() const { return std::string_view{ m_data, m_size }; }
	std::string to_string() const { return std::string{m_data, m_size }; }
	void to_CString(char* out, size_t maxSize) const {
		size_t copyLen = std::min(static_cast<size_t>(m_size), maxSize);
		std::memcpy(out, m_data, copyLen);
		if (copyLen < maxSize)
			out[copyLen] = '\0';
		else if (maxSize > 0)
			out[maxSize - 1] = '\0';
	}

	const char* data() const { return m_data; }
	char* data() { return m_data; }

	operator std::string() const { return to_string(); }

	template <auto U>
		requires UIntMax254MultipleOf4<U>
	bool operator==(const FixedStringBase<U>& rhs) const {
		if (m_size != rhs.m_size)
			return false;

		return _equalData(m_data, rhs.m_data, static_cast<size_t>(m_size));
	}
	bool operator==(const std::string& rhs) const {
		if (m_size != rhs.size())
			return false;
		
		return _equalData(m_data, rhs.data(), static_cast<size_t>(m_size));
	}
	template <auto U>
		requires UIntMax254MultipleOf4<U>
	bool operator!=(const FixedStringBase<U>& rhs) const {
		return !(*this == rhs);
	}

	bool operator!=(const std::string& rhs) const {
		return !(*this == rhs);
	}
};

#endif