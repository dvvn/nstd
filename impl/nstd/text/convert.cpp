module;

#include <cctype>
#include <cwctype>

#include <string>

module nstd.text.convert;

using namespace nstd;

static auto to_lower_chr(const char chr)
{
	const auto lchr = std::tolower(static_cast<std::make_unsigned_t<char>>(chr));
	return static_cast<char>(lchr);
}

static auto to_lower_chr(const wchar_t chr)
{
	const auto lchr = std::towlower(static_cast<std::wint_t>(chr));
	return static_cast<wchar_t>(lchr);
}

template<typename T>
static auto to_lower_impl(const T* ptr)
{
	std::basic_string<T> lstr;
	for (auto chr = ptr; *chr != '\0'; ++chr)
		lstr += to_lower_chr(*chr);
	return lstr;
}

template<typename T>
static auto to_lower_impl(const std::basic_string_view<T> strv)
{
	std::basic_string<T> lstr;
	lstr.reserve(strv.size( ));
	for (const auto chr : strv)
		lstr += to_lower_chr(chr);
	return lstr;
}

nstd::string text::to_lower(const char* str)
{
	return to_lower_impl(str);
}
nstd::string text::to_lower(const std::string_view str)
{
	return to_lower_impl(str);
}
nstd::wstring text::to_lower(const wchar_t* str)
{
	return to_lower_impl(str);
}
nstd::wstring text::to_lower(const std::wstring_view str)
{
	return to_lower_impl(str);
}