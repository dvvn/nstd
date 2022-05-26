module;

#include <cctype>
#include <cwctype>
#include <string>

module nstd.text.convert.to_lower;

static auto _To_lower_chr(const char chr)
{
	const auto lchr = std::tolower(static_cast<int>(chr));
	return static_cast<char>(lchr);
}

static auto _To_lower_chr(const wchar_t chr)
{
	const auto lchr = std::towlower(static_cast<wint_t>(chr));
	return static_cast<wchar_t>(lchr);
}

template <typename T>
static auto _To_lower_impl(const T* ptr)
{
	std::basic_string<T> lstr;
	for (;;)
	{
		const auto chr = *ptr++;
		if (chr == '\0')
			break;
		lstr += _To_lower_chr(chr);
	}
	return lstr;
}

template <typename T>
static auto _To_lower_impl(const std::basic_string_view<T> strv)
{
	std::basic_string<T> lstr;
	lstr.reserve(strv.size());
	for (const auto chr : strv)
		lstr += _To_lower_chr(chr);
	return lstr;
}

std::string to_lower_obj::operator()(const char* str) const
{
	return _To_lower_impl(str);
}

std::string to_lower_obj::operator()(const std::string_view str) const
{
    return _To_lower_impl(str);
}

std::wstring to_lower_obj::operator()(const wchar_t* str) const
{
	return _To_lower_impl(str);
}

std::wstring to_lower_obj::operator()(const std::wstring_view str) const
{
	return _To_lower_impl(str);
}
