module;

#include <nstd/text/convert/unicode.h>

export module nstd.text.convert:unicode;

export namespace nstd::text
{
	template<typename To>
	inline constexpr void* convert_to = nullptr;
}

template<typename To>
struct converter;

#define CVT_OP(_FROM_) string_type operator()(const std::basic_string_view<_FROM_> from) const noexcept;
#define CVT_OP_SELF \
string_view_type operator()(const string_view_type from) const noexcept;\
string_type& operator()(string_type& from) const noexcept;\
string_type operator()(string_type&& from) const noexcept;

#define CVT(_TO_) \
template<>\
struct converter<_TO_>\
{\
	using string_type = std::basic_string<_TO_>;\
	using string_view_type = std::basic_string_view<_TO_>;\
	CVT_##_TO_;\
};\
template<>\
inline constexpr converter<_TO_> nstd::text::convert_to<_TO_>;

#ifdef __cpp_lib_char8_t
CVT(char8_t);
#endif

CVT(char);
CVT(wchar_t);
CVT(char16_t);
CVT(char32_t);

