#pragma once
#include <algorithm>
#include <concepts>

namespace nstd
{
	template <std::floating_point T>
	constexpr T normalize(T value, T min, T max)
	{
		if (value <= min)
			return static_cast<T>(0);
		if (value >= max)
			return static_cast<T>(1);

		return (value - min) / (max - min);
	}

	template <typename Fn, typename From, typename To, typename ...Ts>
	constexpr auto size_cast(From val, Fn checker = {})
	{
		if constexpr (checker(sizeof(From), sizeof(To)))
			return static_cast<To>(val);
		else if constexpr (sizeof...(Ts) > 0)
			return size_cast<Fn, From, Ts...>(val);
		else
			static_assert(std::_Always_false<From>, __FUNCTION__": unknown from type!");
	}

	template <std::integral T>
	constexpr auto to_floating(T value)
	{
		return size_cast<std::less_equal<T>, T, float, double, long double>(value);
	}

	template <std::floating_point T>
	constexpr auto to_integer(T value)
	{
		return size_cast<std::equal_to<T>, T, int64_t, int32_t, int16_t, int8_t>(value);
	}

	/*template <std::floating_point T>
	constexpr T to_floating(T value)
	{
		return value;
	}*/

	template <std::floating_point T>
	constexpr T clamp_by(T val, T step/*, T min, T max*/)
	{
		const auto tmp  = val / step;
		const auto tmp2 = static_cast<T>(to_integer(tmp + static_cast<T>(0.5)));
		return tmp2 * step;
	}

	template <std::integral T>
	constexpr T clamp_by(T val, T step/*, T min, T max*/)
	{
		auto rem = val % step;
		return val - rem;
	}

	enum class scale_mode :uint8_t
	{
		RIGHT
	  , LEFT
	  , AUTO
	};

	template <scale_mode Mode = scale_mode::AUTO, typename T>
	constexpr T scale(T diff, T left, T right)
	{
		if constexpr (Mode == scale_mode::RIGHT)
			return left + (right - left) * diff;
		else if constexpr (Mode == scale_mode::LEFT)
			return right - (right - left) * diff;
		else if constexpr (Mode == scale_mode::AUTO)
		{
			return left < right
					   ? scale<scale_mode::RIGHT>(diff, left, right)
					   : scale<scale_mode::LEFT>(diff, right, left);
		}
		else
		{
			static_assert(__FUNCTION__": unknown mode");
			return left;
		}
	}

	template <scale_mode Mode = scale_mode::AUTO, typename T>
		requires(std::is_arithmetic_v<T>)
	constexpr T scale(T diff, T left, T right, T step)
	{
		const auto val = scale<Mode>(diff, left, right);
		const auto min = [&]
		{
			switch (Mode)
			{
				case scale_mode::RIGHT: return left;
				case scale_mode::LEFT: return right;
				default: return std::min(left, right);
			}
		}( );
		const auto max = [&]
		{
			switch (Mode)
			{
				case scale_mode::RIGHT: return right;
				case scale_mode::LEFT: return left;
				default: return std::max(left, right);
			}
		}( );
		return std::clamp(clamp_by(val, step), min, max);
	}
}
