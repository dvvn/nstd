module;

#include <nstd/text/provide_string.h>
#include <string>

export module nstd.text.string;

template<typename Base>
struct basic_string_wrapper :Base
{
	template<typename ...Args>
	constexpr basic_string_wrapper(Args&&...args)
		:Base(std::forward<Args>(args)...)
	{
	}

#ifndef __cpp_lib_string_contains
	template<typename T>
	constexpr bool contains(const T& obj) const noexcept(!std::is_pointer_v<T>)
	{
		return this->find(obj) != this->npos;
	}
#endif
};

#define NSTD_STRING_DECLARE_IMPL(_TYPE_,_PREFIX_,_POSTFIX_)\
	using _PREFIX_##string##_POSTFIX_ = ::basic_string_wrapper<std::basic_string##_POSTFIX_<_TYPE_>>;

export namespace nstd::inline text
{
	NSTD_STRING_DECLARE;
}

