module;
#include "type_traits.h"
#include "runtime_assert.h"

#include <string>
#include <functional>

#ifdef _DEBUG
#include <stdexcept>
#endif

export module nstd:hashed_string;

namespace nstd
{
#define NSTD_HASHED_STRING_WRAP(_NAME_) \
	private:\
		using Base::_NAME_;\
	public:\
		template<typename ...Args>\
	/*requires requires(Base &base,Args&&...args) { base._NAME_(std::forward<Args>(args)...); }*/\
	decltype(auto) _NAME_(Args&&...args)\
	{\
		if constexpr(std::is_void_v<decltype(Base::_NAME_(std::forward<Args>(args)...))>)\
		{\
			Base::_NAME_(std::forward<Args>(args)...);\
			this->_Calc_hash( );\
		}\
		else\
		{\
			auto ret = Base::_NAME_(std::forward<Args>(args)...);\
			this->_Calc_hash( );\
			return ret;\
		}\
	}

	template<class Base, template<typename> class Hasher = std::hash>
	struct hashed_string_wrapper : Base
	{
		using hash_type = /*decltype(std::invoke(std::declval<Hasher>( ), std::declval<Base>( )))*/size_t;
		using hash_func_type = Hasher<Base>;

	private:
		hash_type hash_ = static_cast<hash_type>(-1);
		[[no_unique_address]] hash_func_type hasher_;

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

		template<class Base2, template<typename> class Hasher2>
		constexpr void _Write_hash(const hashed_string_wrapper<Base2, Hasher2>& holder)
		{
			if constexpr (same_template<Hasher, Hasher2>( ))
			{
				if constexpr (std::same_as<Base, Base2>)
					hash_ = holder.hash( );
				else
					_Write_hash(holder.hash( ));
			}
			else
			{
				_Calc_hash( );
			}
		}

	public:
		static_assert(std::is_empty_v<hash_func_type>, "Hasher class must be empty");

		constexpr hashed_string_wrapper( ) = default;

		template<class Base2, template<typename> class Hasher2>
			requires(std::constructible_from<Base, Base2>)
		constexpr hashed_string_wrapper(const hashed_string_wrapper<Base2, Hasher2>& other) :Base(static_cast<const Base2&>(other))
		{
			_Write_hash(other);
		}

		template<class Base2, template<typename> class Hasher2>
			requires(std::constructible_from<Base, Base2>)
		constexpr hashed_string_wrapper(hashed_string_wrapper<Base2, Hasher2>&& other) :Base(static_cast<Base2&&>(other))
		{
			_Write_hash(other);
		}

		template<typename ...T>
			requires(std::constructible_from<Base, T...>)
		constexpr hashed_string_wrapper(T&&...val) :Base(std::forward<T>(val)...)
		{
			_Calc_hash( );
		}

		template<typename ...T>
			requires(std::constructible_from<Base, T...>)
		constexpr hashed_string_wrapper(T&& ...val, hash_type hash) :Base(std::forward<T>(val)...)
		{
			_Write_hash(hash);
		}

		/*template<typename Itr>
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
		}*/

		constexpr hash_func_type get_hasher( )const { return hasher_; }
		constexpr hash_type hash( )const { return hash_; }

		NSTD_HASHED_STRING_WRAP(substr);
	};

	template<typename Chr
		, template<typename> class Hasher = std::hash
		, class Traits = std::char_traits<Chr>
		, class Base = hashed_string_wrapper<std::basic_string_view<Chr, Traits>, Hasher>>
		struct basic_hashed_string_view : Base
	{
		using Base::Base;

		NSTD_HASHED_STRING_WRAP(remove_prefix);
		NSTD_HASHED_STRING_WRAP(remove_suffix);
	private:
#if __cplusplus > 201703L
		using Base::swap;
#endif
	};

	template<typename Chr
		, template<typename> class Hasher = std::hash
		, class Traits = std::char_traits<Chr>
		, class Allocator = std::allocator<Chr>
		, class Base = hashed_string_wrapper<std::basic_string<Chr, Traits, Allocator>, Hasher>>
		struct basic_hashed_string : Base
	{
		using Base::Base;

		NSTD_HASHED_STRING_WRAP(clear);
		NSTD_HASHED_STRING_WRAP(insert);
		NSTD_HASHED_STRING_WRAP(erase);
		NSTD_HASHED_STRING_WRAP(push_back);
		NSTD_HASHED_STRING_WRAP(pop_back);
		NSTD_HASHED_STRING_WRAP(append);
		NSTD_HASHED_STRING_WRAP(operator+=);
		NSTD_HASHED_STRING_WRAP(replace);
		NSTD_HASHED_STRING_WRAP(resize);
#if __cplusplus > 202002L
		NSTD_HASHED_STRING_WRAP(resize_and_overwrite);
#endif
	private:
		using Base::swap;
	};
}

export namespace nstd
{
#define NSTD_USE_HASHED_STRING_SPEC(_TYPE_,_PREFIX_,_POSTFIX_)\
	using hashed_##_PREFIX_##string##_POSTFIX_ = basic_hashed_string##_POSTFIX_##<_TYPE_>;

#define NSTD_USE_HASHED_STRING(_TYPE_,...)\
	NSTD_USE_HASHED_STRING_SPEC(_TYPE_,__VA_ARGS__,)\
	NSTD_USE_HASHED_STRING_SPEC(_TYPE_,__VA_ARGS__,_view)

	NSTD_USE_HASHED_STRING(char);
	NSTD_USE_HASHED_STRING(char16_t, u16);
	NSTD_USE_HASHED_STRING(char32_t, u32);
	NSTD_USE_HASHED_STRING(wchar_t, w);
#ifdef __cpp_lib_char8_t
	NSTD_USE_HASHED_STRING(char8_t, u8);
#endif

	/*using hashed_string_view = basic_hashed_string_view<char>;
	using hashed_wstring_view = basic_hashed_string_view<wchar_t>;

	using hashed_string = basic_hashed_string<char>;
	using hashed_wstring = basic_hashed_string<wchar_t>;*/

#define NSTD_HASHED_STRING_OPERATOR_IMPL(_CHECK_,_RET_,_OP_)\
	template<class Base, template<typename> class Hasher\
		, class Base2, template<typename> class Hasher2>\
		requires(_CHECK_<Base, Base2> && nstd::same_template<Hasher, Hasher2>( ))\
	constexpr _RET_ operator _OP_(const hashed_string_wrapper<Base, Hasher>& l, const hashed_string_wrapper<Base2, Hasher2>& r)\
	{\
		return l.hash( ) _OP_ r.hash( );\
	}

#define NSTD_HASHED_STRING_OPERATOR(_OP_) \
NSTD_HASHED_STRING_OPERATOR_IMPL(std::equality_comparable_with,bool,_OP_);

	NSTD_HASHED_STRING_OPERATOR(== );
	NSTD_HASHED_STRING_OPERATOR(!= );
}