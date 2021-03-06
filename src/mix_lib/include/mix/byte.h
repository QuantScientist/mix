#pragma once
#include <mix/exceptions.h>

#include <core/type_utils.h>

#include <ostream>

#include <climits>

namespace mix {

class Byte
{
public:
	static constexpr unsigned int k_bits_count = 6;

	using NarrowType = unsigned char;

	static_assert(sizeof(NarrowType) * CHAR_BIT >= k_bits_count,
		"Selected type for Byte representation can't "
		"hold number with given bits count");

	static constexpr NarrowType k_min_value = 0;
	static constexpr NarrowType k_values_count = (1 << k_bits_count);
	static constexpr NarrowType k_max_value = (1 << k_bits_count) - 1;

private:
	template<typename Number>
	static constexpr bool CanBeNarrowedToByte =
		std::is_integral<Number>::value &&
		!std::is_same<Number, bool>::value;

	template<typename Number>
	static constexpr bool CanBeConvertedTo =
		CanBeNarrowedToByte<Number>;

public:
	Byte()
		: Byte{0}
	{
	}

	template<typename Number,
		typename = std::enable_if_t<CanBeNarrowedToByte<Number>>>
	Byte(Number value)
		: value_{}
	{
		set(value);
	}

	template<typename Number,
		typename = std::enable_if_t<CanBeNarrowedToByte<Number>>>
	void set(Number value)
	{
		if (!CanHoldValue(value))
		{
			throw ByteOverflow{};
		}
		value_ = static_cast<NarrowType>(value);
	}

	NarrowType value() const
	{
		return value_;
	}

	template<typename Number,
		typename = std::enable_if_t<CanBeConvertedTo<Number>>>
	Number cast_to() const
	{
		return static_cast<Number>(value());
	}

	template<typename Number,
		typename = std::enable_if_t<CanBeNarrowedToByte<Number>>>
	static bool CanHoldValue(Number value)
	{
		return (value >= static_cast<Number>(k_min_value)) &&
			(value <= static_cast<Number>(k_max_value));
	}

	static Byte Min()
	{
		return k_min_value;
	}

	static Byte Max()
	{
		return k_max_value;
	}

private:
	NarrowType value_;
};

inline bool operator==(const Byte& lhs, const Byte& rhs)
{
	return (lhs.value() == rhs.value());
}

inline bool operator!=(const Byte& lhs, const Byte& rhs)
{
	return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& o, const Byte& b)
{
	o << +b.value();
	return o;
}

} // namespace mix



