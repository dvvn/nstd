#pragma once
#include <type_traits>

namespace nstd
{
	template <typename T>
	union decayed_enum
	{
		using value_type = T;
		using raw_type = std::underlying_type_t<T>;

		value_type val;
		raw_type raw;

		operator value_type( ) const
		{
			return val;
		}

		operator raw_type( ) const
		{
			return raw;
		}
	};

	template <typename T>
	union decayed_enum_fake
	{
		using value_type = T;
		value_type val, raw;

		operator value_type( ) const
		{
			return val;
		}
	};

	template <class T>
	_INLINE_VAR constexpr bool is_decayed_enum_v = false;

	template <class T>
	_INLINE_VAR constexpr bool is_decayed_enum_v<decayed_enum<T>> = true;

	template <class T>
	_INLINE_VAR constexpr bool is_decayed_enum_v<decayed_enum_fake<T>> = true;

	namespace detail
	{
		template <typename V, typename T>
		decltype(auto) ref_or_copy(T&& val)
		{
			if constexpr (!std::is_const_v<std::remove_reference_t<T>> && std::is_lvalue_reference_v<T>)
				return reinterpret_cast<V&>(val);
			else
			{
				auto magic  = reinterpret_cast<const void*>(std::addressof(val));
				auto magic2 = static_cast<const V*>(magic);
				return *magic2;
			}
		}
	}

	template <typename T>
	decltype(auto) unwrap_enum_impl(T val)
	{
		using raw_t = std::remove_cvref_t<T>;

		if constexpr (is_decayed_enum_v<raw_t>)
			return detail::ref_or_copy<raw_t>(std::forward<T>(val));
		else if constexpr (std::is_enum_v<raw_t>)
			return detail::ref_or_copy<decayed_enum<raw_t>>(std::forward<T>(val));
		else
			return detail::ref_or_copy<decayed_enum_fake<raw_t>>(std::forward<T>(val));
	}

	template <typename T>
	decltype(auto) unwrap_enum(T&& val)
	{
		using type = decltype(val);
		return unwrap_enum_impl<type>(static_cast<type>(val));
	}

	namespace enum_operators
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
		concept bit_logical_not = requires(T t) { !t; };

		template <typename T>
		_INLINE_VAR constexpr bool enum_or_integral_v = std::is_enum_v<T> || std::is_convertible_v<T, size_t>;

		template <typename T>
		_INLINE_VAR constexpr bool enum_or_integral_v<decayed_enum_fake<T>> = true;

		template <typename T>
		_INLINE_VAR constexpr bool enum_or_integral_v<decayed_enum<T>> = true;

		template <typename T>
		concept enum_or_integral = (enum_or_integral_v<std::remove_cvref_t<T>>);

		//-----

		template <typename L, typename R>
		decltype(auto) _Bit_and(L l, R r)
		{
			decltype(auto) lu = unwrap_enum(l);
			lu.raw &= unwrap_enum_impl(r).raw;
			return lu;
		}

		template <enum_or_integral L, enum_or_integral R>
			requires(!bit_and_assign<L, R>)
		auto& operator&=(L& l, R r)
		{
			return _Bit_and<L&>(l, r);
		}

		template <enum_or_integral L, enum_or_integral R>
			requires(!bit_and<L, R>)
		auto operator&(L l, R r)
		{
			return _Bit_and<L>(l, r);
		}

		template <typename L, typename R>
		decltype(auto) _Bit_or(L l, R r)
		{
			decltype(auto) lu = unwrap_enum(l);
			lu.raw |= unwrap_enum_impl(r).raw;
			return lu;
		}

		template <enum_or_integral L, enum_or_integral R>
			requires(!bit_or_assign<L, R>)
		auto& operator|=(L& l, R r)
		{
			return _Bit_or<L&>(l, r);
		}

		template <enum_or_integral L, enum_or_integral R>
			requires(!bit_or<L, R>)
		auto operator|(L l, R r)
		{
			return _Bit_or<L>(l, r);
		}

		template <typename L, typename R>
		decltype(auto) _Bit_xor(L l, R r)
		{
			decltype(auto) lu = unwrap_enum(l);
			lu.raw ^= unwrap_enum_impl(r).raw;
			return lu;
		}

		template <enum_or_integral L, enum_or_integral R>
			requires(!bit_xor_assign<L, R>)
		auto& operator^=(L& l, R r)
		{
			return _Bit_xor<L&>(l, r);
		}

		template <enum_or_integral L, enum_or_integral R>
			requires(!bit_xor<L, R>)
		auto operator^(L l, R r)
		{
			return _Bit_xor<L>(l, r);
		}

		template <enum_or_integral T>
			requires(!bit_not<T>)
		auto operator~(T t)
		{
			auto tmp = unwrap_enum(t);
			tmp.raw  = ~tmp.raw;
			return tmp;
		}

		template <enum_or_integral T>
			requires(!bit_logical_not<T>)
		bool operator!(T t)
		{
			auto tmp = unwrap_enum_impl(t);
			return !tmp.raw;
		}
	}
}
