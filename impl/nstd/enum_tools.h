#pragma once
#include <functional>

namespace nstd::enum_operators
{
	template <typename A, typename B = A>
	concept bit_and = requires(A a, B b) { a & b; };

	template <typename A, typename B = A>
	concept bit_and_assign = requires(A& a, B b) { a &= b; };

	template <typename A, typename B = A>
	concept bit_or = requires(A a, B b) { a | b; };

	template <typename A, typename B = A>
	concept bit_or_assign = requires(A& a, B b) { a |= b; };

	template <typename A, typename B = A>
	concept bit_xor = requires(A a, B b) { a ^ b; };

	template <typename A, typename B = A>
	concept bit_xor_assign = requires(A& a, B b) { a ^= b; };

	template <typename A, typename B = A>
	concept bit_lshift = requires(A a, B b) { a << b; };

	template <typename A, typename B = A>
	concept bit_rshift = requires(A a, B b) { a >> b; };

	template <typename T>
	concept bit_not = requires(T t) { ~t; };

	template <typename T>
	concept enum_or_integral = (std::is_enum_v<T> || std::is_convertible_v<T, size_t>);

	template <typename T>
	union decayed_enum
	{
		T val;
		std::underlying_type_t<T> raw;
	};

	template <typename T>
	union decayed_enum_fake
	{
		T val, raw;
	};

	template <typename V, typename T>
	decltype(auto) ref_or_copy(T&& val)
	{
		if constexpr (!std::is_const_v<std::remove_reference_t<T>> && std::is_lvalue_reference_v<T>)
			return reinterpret_cast<V&>(val);
		else
			return reinterpret_cast<V>(val);
	}

	template <typename T>
	decltype(auto) unwrap_enum(T&& val)
	{
		using raw_t = std::remove_cvref_t<T>;

		if constexpr (std::is_enum_v<raw_t>)
			return ref_or_copy<decayed_enum<raw_t>>(std::forward<T>(val));
		else
			return ref_or_copy<decayed_enum_fake<raw_t>>(std::forward<T>(val));
	}

	template <typename Fn, typename ...Ts>
	decltype(auto) do_bit_fn(Fn&& fn, Ts&&...ts)
	{
		auto tpl = std::forward_as_tuple(unwrap_enum(std::forward<Ts>(ts))...);

		auto& first = std::get<0>(tpl);

		const auto fn_proxy = [&]<typename ...Tsr>(Tsr&& ...tsr)
		{
			first.raw = std::invoke(fn, tsr.raw...);
		};

		std::apply(fn_proxy, tpl);

		using value_type = decltype(first.val);
		using raw_type = decltype(first.raw);

		constexpr bool return_ref = std::is_lvalue_reference_v<std::tuple_element_t<0, decltype(tpl)>>;

		if constexpr (std::convertible_to<value_type, raw_type>)
		{
			if constexpr (return_ref)
				return static_cast<value_type&>(first.val);
			else
				return first.val;
		}
		else
		{
			if constexpr (return_ref)
				return static_cast<raw_type&>(first.raw);
			else
				return first.raw;
		}
	}

	//-----

	template <enum_or_integral L, enum_or_integral R>
		requires(!bit_and_assign<L, R>)
	auto& operator&=(L& l, R r)
	{
		return do_bit_fn(std::bit_and( ), l, r);
	}

	template <enum_or_integral L, enum_or_integral R>
		requires(!bit_and<L, R>)
	auto operator&(L l, R r)
	{
		return do_bit_fn(std::bit_and( ), l, r);
	}

	template <enum_or_integral L, enum_or_integral R>
		requires(!bit_or_assign<L, R>)
	auto& operator|=(L& l, R r)
	{
		return do_bit_fn(std::bit_or( ), l, r);
	}

	template <enum_or_integral L, enum_or_integral R>
		requires(!bit_or<L, R>)
	auto operator|(L l, R r)
	{
		return do_bit_fn(std::bit_or( ), l, r);
	}

	template <enum_or_integral L, enum_or_integral R>
		requires(!bit_xor_assign<L, R>)
	auto& operator^=(L l, R r)
	{
		return do_bit_fn(std::bit_xor( ), l, r);
	}

	template <enum_or_integral L, enum_or_integral R>
		requires(!bit_xor<L, R>)
	auto operator^(L l, R r)
	{
		return do_bit_fn(std::bit_xor( ), l, r);
	}

	template <enum_or_integral T>
		requires(!bit_not<T>)
	auto operator~(T t)
	{
		return do_bit_fn(std::bit_not( ), t);
	}
}
