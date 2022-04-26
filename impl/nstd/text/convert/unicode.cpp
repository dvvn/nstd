module;

#include <nstd/text/convert/unicode.h>

#include <ww898/utf_converters.hpp>

module nstd.text.convert:unicode;

using ww898::utf::detail::utf_selector;

template<typename To, typename From>
static auto _Conv(const std::basic_string_view<From> from) noexcept
{
	const auto bg = from.data();
	const auto size = from.size();
	const auto ed = bg + size;

	std::basic_string<To> buff;
	buff.reserve(size);

	using namespace ww898::utf;
	conv<utf_selector_t<From>, utf_selector_t<To>>(bg, ed, std::back_inserter(buff));

	return buff;
}

#ifdef _DEBUG
static void _Direct_trap() noexcept
{
	[[maybe_unused]]
	const char dummy = 1;
}
#define DIRECT_TRAP _Direct_trap()
#else
#define DIRECT_TRAP (void)0
#endif

#define OP(_TO_)\
auto converter<_TO_>::operator()

#define CVT_OP_IMPL(_TO_,_FROM_)\
OP(_TO_)(const std::basic_string_view<_FROM_> from) const noexcept -> string_type\
{\
	return _Conv<_TO_>(from);\
}

#define CVT_OP_SELF_IMPL(_TO_) \
OP(_TO_)(const string_view_type from) const noexcept -> string_view_type { DIRECT_TRAP; return from; }\
OP(_TO_)(string_type& from) const noexcept -> string_type& { DIRECT_TRAP; return from; }\
OP(_TO_)(string_type&& from) const noexcept -> string_type { DIRECT_TRAP; return std::move(from); }\

#ifdef __cpp_lib_char8_t
template < >
struct utf_selector<char8_t> final
{
	using type = utf8;
};

#define CVT_OP(_TO_) CVT_OP_IMPL(char8_t,_TO_)
#define CVT_OP_SELF CVT_OP_SELF_IMPL(char8_t)
CVT_char8_t
#endif

#pragma warning(push)
#pragma warning(disable:4005)

#define CVT_OP(_TO_) CVT_OP_IMPL(char,_TO_)
#define CVT_OP_SELF CVT_OP_SELF_IMPL(char)
CVT_char

#define CVT_OP(_TO_) CVT_OP_IMPL(wchar_t,_TO_)
#define CVT_OP_SELF CVT_OP_SELF_IMPL(wchar_t)
CVT_wchar_t

#define CVT_OP(_TO_) CVT_OP_IMPL(char16_t,_TO_)
#define CVT_OP_SELF CVT_OP_SELF_IMPL(char16_t)
CVT_char16_t

#define CVT_OP(_TO_) CVT_OP_IMPL(char32_t,_TO_)
#define CVT_OP_SELF CVT_OP_SELF_IMPL(char32_t)
CVT_char32_t

#pragma warning(pop)