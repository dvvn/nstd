module;

#include <nstd/type_traits.h>
#include <nstd/runtime_assert.h>

#include <string>
#include <functional>
#include <optional>

#ifdef _DEBUG
#include <stdexcept>
#endif

export module nstd.text.hashed_string;

template<typename Fn>
concept func_maybe_null = requires(const Fn fn)
{
	!fn;
};

template<typename Fn>
class lazy_invoke
{
public:
	using value_type = std::conditional_t<func_maybe_null<Fn>, Fn, std::optional<Fn>>;
	using func_type = Fn;

	constexpr ~lazy_invoke( )
	{
		if (!fn_)
			return;
		if constexpr (!func_maybe_null<Fn>)
			std::invoke(*fn_);
		else
			std::invoke(fn_);
	}

	template<typename Fn1>
	constexpr lazy_invoke(Fn1&& fn) :fn_(std::forward<Fn1>(fn))
	{
	}

	constexpr lazy_invoke(lazy_invoke&& other)noexcept
	{
		*this = std::move(other);
	}

	constexpr lazy_invoke& operator=(lazy_invoke&& other)noexcept
	{
		using std::swap;
		swap(fn_, other.fn_);
	}
private:

	value_type fn_;
};

template<typename Fn>
lazy_invoke(const Fn&)->lazy_invoke<Fn>;

#define NSTD_COMMA ,

#define NSTD_HASHED_STRING_CALL_ORIGINAL(_NAME_)\
	Base::_NAME_(std::forward<Args>(args)...)

#define NSTD_HASHED_STRING_WRAP(_RET_,_NAME_,_PROXY_,...) \
	private:\
		using Base::_NAME_;\
	public:\
		template<typename ...Args>\
		_RET_ _NAME_(Args&&...args)\
		{\
			const lazy_invoke lazy = [=]{this->_Calc_hash();};\
			_PROXY_ NSTD_HASHED_STRING_CALL_ORIGINAL(_NAME_);\
			__VA_ARGS__;\
		}

#define NSTD_HASHED_STRING_WRAP_VOID(_NAME_)\
	NSTD_HASHED_STRING_WRAP(void,_NAME_,)

#define NSTD_HASHED_STRING_WRAP_THIS(_NAME_)\
	NSTD_HASHED_STRING_WRAP(auto&,_NAME_,,return *this)

template<class Base, typename T, typename Q>
decltype(auto) this_or_iterator(T* thisptr, const Q& proxy_result)
{
	if constexpr (std::same_as<Q, Base>)
	{
		//return *this instead of *Base
		return *thisptr;
	}
	else
	{
		//return the iterator
		return proxy_result;
	}
}

#define NSTD_HASHED_STRING_WRAP_THIS_OR_ITERATOR(_NAME_)\
	NSTD_HASHED_STRING_WRAP(decltype(auto), _NAME_, decltype(auto) result =, return this_or_iterator<Base>(this NSTD_COMMA result))

template<class Base, template<typename> class Hasher = std::hash>
struct hashed_string_wrapper : Base
{
	using hash_type = /*decltype(std::invoke(std::declval<Hasher>( ), std::declval<Base>( )))*/size_t;
	using hash_func_type = Hasher<Base>;

	static_assert(std::is_empty_v<hash_func_type>, "Hasher class must be empty");

private:
	hash_type hash_;
	[[no_unique_address]] hash_func_type hasher_;

	constexpr hash_type _Get_hash( )const
	{
		return std::invoke(hasher_, *static_cast<const Base*>(this));
	}

	constexpr void _Validate_hash(hash_type hash)const
	{
#ifdef _DEBUG
		if (_Get_hash( ) == hash)
			return;
		if (std::is_constant_evaluated( ))
			throw std::logic_error("Incorrect hash passed");
		else
			runtime_assert("Incorrect hash passed");
#endif
	}

	constexpr void _Calc_hash( )
	{
		hash_ = _Get_hash( );
	}

	template<class Base2, template<typename> class Hasher2>
	constexpr void _Try_write_hash(const hashed_string_wrapper<Base2, Hasher2>& holder)
	{
		static_assert(std::constructible_from<Base, Base2>);

		if constexpr (!same_template<Hasher, Hasher2>( ))
		{
			_Calc_hash( );
		}
		else
		{
			using val1_t = decltype(std::declval<Base>( )[0]);
			using val2_t = decltype(std::declval<Base2>( )[0]);

			if constexpr (std::same_as<val1_t, val2_t>)
				hash_ = holder.hash_;
			else
				_Write_hash(holder.hash_);
		}
	}

	constexpr void _Write_hash(hash_type hash)
	{
		_Validate_hash(hash);
		hash_ = hash;
	}
public:

	constexpr hashed_string_wrapper( )
	{
		_Calc_hash( );
	}

	template<class Base2, template<typename> class Hasher2>
		requires(std::constructible_from<Base, Base2>)
	constexpr hashed_string_wrapper(const hashed_string_wrapper<Base2, Hasher2>& other) :Base(static_cast<const Base2&>(other))
	{
		_Try_write_hash(other);
	}

	template<class Base2, template<typename> class Hasher2>
		requires(std::constructible_from<Base, Base2>)
	constexpr hashed_string_wrapper(hashed_string_wrapper<Base2, Hasher2>&& other) :Base(static_cast<Base2&&>(other))
	{
		_Try_write_hash(other);
	}

	template<class Base2>
		requires(std::constructible_from<Base, Base2>)
	constexpr hashed_string_wrapper(Base2&& other) :Base(std::forward<Base2>(other))
	{
		_Calc_hash( );
	}

	template<class Base2>
		requires(std::constructible_from<Base, Base2>)
	constexpr hashed_string_wrapper(Base2&& other, hash_type hash) :Base(std::forward<Base2>(other))
	{
		_Write_hash(hash);
	}

	template<typename Itr>
	constexpr hashed_string_wrapper(Itr bg, Itr ed) :Base(bg, ed)
	{
		_Calc_hash( );
	}

	template<typename Itr>
	constexpr hashed_string_wrapper(Itr bg, Itr ed, hash_type hash) :Base(bg, ed)
	{
		_Write_hash(hash);
	}

	constexpr hash_func_type hash_function( )const { return hasher_; }
	constexpr hash_type hash( )const { return hash_; }

};

template<typename Chr
	, template<typename> class Hasher = std::hash
	, class Traits = std::char_traits<Chr>
	, class Base = hashed_string_wrapper<std::basic_string_view<Chr, Traits>, Hasher>>
	struct basic_hashed_string_view : Base
{
	using Base::Base;

	NSTD_HASHED_STRING_WRAP_THIS(substr);

	NSTD_HASHED_STRING_WRAP_VOID(remove_prefix);
	NSTD_HASHED_STRING_WRAP_VOID(remove_suffix);

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

	NSTD_HASHED_STRING_WRAP_THIS(substr);

	NSTD_HASHED_STRING_WRAP_THIS(assign);
	NSTD_HASHED_STRING_WRAP_VOID(clear);
	NSTD_HASHED_STRING_WRAP_THIS_OR_ITERATOR(insert);
	NSTD_HASHED_STRING_WRAP_THIS_OR_ITERATOR(erase);
	NSTD_HASHED_STRING_WRAP_VOID(push_back);
	NSTD_HASHED_STRING_WRAP_VOID(pop_back);
	NSTD_HASHED_STRING_WRAP_THIS(append);
	NSTD_HASHED_STRING_WRAP_THIS(operator+=);
	NSTD_HASHED_STRING_WRAP_THIS(replace);
	NSTD_HASHED_STRING_WRAP_VOID(resize);
#if __cplusplus > 202002L
	NSTD_HASHED_STRING_WRAP_VOID(resize_and_overwrite);
#endif
private:
	using Base::swap;
};

export namespace nstd::inline text
{
#define NSTD_USE_HASHED_STRING_SPEC(_TYPE_,_PREFIX_,_POSTFIX_)\
	using hashed_##_PREFIX_##string##_POSTFIX_ = basic_hashed_string##_POSTFIX_##<_TYPE_>;

#define NSTD_USE_HASHED_STRING(_TYPE_,_PREFIX_)\
	NSTD_USE_HASHED_STRING_SPEC(_TYPE_,_PREFIX_,)\
	NSTD_USE_HASHED_STRING_SPEC(_TYPE_,_PREFIX_,_view)

	NSTD_USE_HASHED_STRING(char,);
	NSTD_USE_HASHED_STRING(char16_t, u16);
	NSTD_USE_HASHED_STRING(char32_t, u32);
	NSTD_USE_HASHED_STRING(wchar_t, w);
#ifdef __cpp_lib_char8_t
	NSTD_USE_HASHED_STRING(char8_t, u8);
#endif

#define NSTD_HASHED_STRING_OPERATOR_SIMPLE_HEAD \
	template<class Base, template<typename> class Hasher\
		, class Wrapper = hashed_string_wrapper<Base, Hasher>\
		, typename HashType = Wrapper::hash_type>

#define NSTD_HASHED_STRING_OPERATOR_IMPL(_CHECK_,_RET_,_OP_)\
	template<class Base, template<typename> class Hasher\
		, class Base2, template<typename> class Hasher2>\
	constexpr _RET_ operator _OP_(const hashed_string_wrapper<Base, Hasher>& l, const hashed_string_wrapper<Base2, Hasher2>& r)\
	{\
		static_assert(std::equality_comparable_with<Base, Base2>, __FUNCTION__": Base classes must be equality comparable!");\
		static_assert(nstd::same_template<Hasher, Hasher2>( ), __FUNCTION__": Hash functions must be have same base!");\
		return l.hash( ) _OP_ r.hash( );\
	}\
	NSTD_HASHED_STRING_OPERATOR_SIMPLE_HEAD\
	constexpr _RET_ operator _OP_(const Wrapper& l, HashType hash)\
	{\
		return l.hash( ) _OP_ hash;\
	}\
	NSTD_HASHED_STRING_OPERATOR_SIMPLE_HEAD\
	constexpr _RET_ operator _OP_(HashType hash, const Wrapper& r)\
	{\
		return hash _OP_ r.hash( );\
	}

#define NSTD_HASHED_STRING_OPERATOR(_OP_) \
NSTD_HASHED_STRING_OPERATOR_IMPL(std::equality_comparable_with,bool,_OP_);

	NSTD_HASHED_STRING_OPERATOR(== );
	NSTD_HASHED_STRING_OPERATOR(!= );
}