#pragma once

#ifdef _DEBUG
#include "mem/address_includes.h"
#endif
#include "nstd/type_traits.h"
#include <string>
#include <functional>

namespace nstd
{
	template<class Base, class Hasher = std::hash<Base>>
	struct hashed_string_wrapper : Base
	{
		using hash_type = /*decltype(std::invoke(std::declval<Hasher>( ), std::declval<Base>( )))*/size_t;
		using hash_func_type = Hasher;

	private:
		hash_type hash_ = static_cast<hash_type>(-1);
		[[no_unique_address]] Hasher hasher_;

		constexpr void _Validate_hash(const Base& str, hash_type hash)const
		{
#ifdef _DEBUG
			hash_type hash2 = std::invoke(hasher_, str);
			if (hash2 == hash)
				return;
			if (std::is_constant_evaluated( ))
				throw std::logic_error("Incorrect hash passed");
			else
				runtime_assert("Incorrect hash passed");
#endif
		}

		constexpr void _Calc_hash( )
		{
			hash_ = std::invoke(hasher_, *static_cast<Base*>(this));
		}

		constexpr void _Write_hash(hash_type hash)
		{
			_Validate_hash(*static_cast<Base*>(this), hash);
			hash_ = hash;
		}

		template<class Base2, class Hasher2>
		constexpr void _Write_hash(const hashed_string_wrapper<Base2, Hasher2>& holder)
		{
			hash_ = holder.hash_;
		}

	public:
		static_assert(std::is_empty_v<Hasher>, "Hasher class must be empty");

		constexpr hashed_string_wrapper( ) = default;

		template<class Base2, class Hasher2>
			requires(std::constructible_from<Base, Base2>&& same_template<Hasher, Hasher2>( ))
		constexpr hashed_string_wrapper(const hashed_string_wrapper<Base2, Hasher2>& other) :Base(static_cast<const Base2&>(other))
		{
			_Write_hash(other);
		}

		template<class Base2, class Hasher2>
			requires(std::constructible_from<Base, Base2>&& same_template<Hasher, Hasher2>( ))
		constexpr hashed_string_wrapper(hashed_string_wrapper<Base2, Hasher2>&& other) :Base(static_cast<Base2&&>(other))
		{
			_Write_hash(other);
		}

		template<typename T>
			requires(std::constructible_from<Base, T>)
		constexpr hashed_string_wrapper(T&& val) :Base(std::forward<T>(val))
		{
			_Calc_hash( );
		}

		template<typename T>
			requires(std::constructible_from<Base, T>)
		constexpr hashed_string_wrapper(T&& val, hash_type hash) :Base(std::forward<T>(val))
		{
			_Write_hash(hash);
		}

		template<typename Itr>
			requires(std::constructible_from<Base, Itr, Itr>)
		constexpr hashed_string_wrapper(Itr bg, Itr ed) :Base(bg, ed)
		{
			_Calc_hash( );
		}

		template<typename Itr>
			requires(std::constructible_from<Base, Itr, Itr>)
		constexpr hashed_string_wrapper(Itr bg, Itr ed, hash_type hash) :Base(bg, ed)
		{
			_Write_hash(hash);
		}

		constexpr hash_func_type get_hasher( )const { return hasher_; }
		constexpr hash_type hash( )const { return hash_; }
	};

	template<class Base, class Hasher, class Base2, class Hasher2 >
		requires(std::equality_comparable_with<Base, Base2>&& nstd::same_template<Hasher, Hasher2>( ))
	constexpr bool operator ==(const hashed_string_wrapper<Base, Hasher>& l, const hashed_string_wrapper<Base2, Hasher2>& r)
	{
		return l.hash( ) == r.hash( );
	}

	template<typename Chr
		, class Traits = std::char_traits<Chr>
		, class Hasher = std::hash<std::basic_string_view<Chr, Traits>>
		, class Base = hashed_string_wrapper<std::basic_string_view<Chr, Traits>, Hasher>>
		struct basic_hashed_string_view : Base
	{
		using Base::Base;
	};

	using hashed_string_view = basic_hashed_string_view<char>;
	using hashed_wstring_view = basic_hashed_string_view<wchar_t>;

	template<typename Chr
		, class Traits = std::char_traits<Chr>
		, class Allocator = std::allocator<Chr>
		, class Hasher = std::hash<std::basic_string<Chr, Traits, Allocator>>
		, class Base = hashed_string_wrapper<std::basic_string<Chr, Traits, Allocator>, Hasher>>
		struct basic_hashed_string : Base
	{
		using Base::Base;
	};

	using hashed_string = basic_hashed_string<char>;
	using hashed_wstring = basic_hashed_string<wchar_t>;
}