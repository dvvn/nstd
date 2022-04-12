module;

#include <cctype>
#include <cwctype>

#include <string>

module nstd.text.convert;

static auto to_lower_chr(const char chr) noexcept
{
	const auto lchr = std::tolower(static_cast<std::make_unsigned_t<char>>(chr));
	return static_cast<char>(lchr);
}

static auto to_lower_chr(const wchar_t chr) noexcept
{
	const auto lchr = std::towlower(static_cast<std::wint_t>(chr));
	return static_cast<wchar_t>(lchr);
}

template<typename T>
static auto to_lower_impl(const T* ptr) noexcept
{
	std::basic_string<T> lstr;
	for (auto chr = ptr; *chr != '\0'; ++chr)
		lstr += to_lower_chr(*chr);
	return lstr;
}

template<typename T>
static auto to_lower_impl(const std::basic_string_view<T> strv) noexcept
{
	std::basic_string<T> lstr;
	lstr.reserve(strv.size( ));
	for (const auto chr : strv)
		lstr += to_lower_chr(chr);
	return lstr;
}

std::string to_lower_obj::operator()(const char* str) const noexcept
{
	return to_lower_impl(str);
}
std::string to_lower_obj::operator()(const std::string_view str) const noexcept
{
	return to_lower_impl(str);
}
std::wstring to_lower_obj::operator()(const wchar_t* str) const noexcept
{
	return to_lower_impl(str);
}
std::wstring to_lower_obj::operator()(const std::wstring_view str) const noexcept
{
	return to_lower_impl(str);
}