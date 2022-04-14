#pragma once

#ifdef _UNICODE
#include <ww898/utf_converters.hpp>

template<typename T>
auto _To_bytes(const std::basic_string_view<T> str) noexcept
{
	if constexpr (std::is_same_v<T, char>)
		return str;
	else
		return ww898::utf::conv<char>(str);
}

template<typename T>
auto _To_wide(const std::basic_string_view<T> str) noexcept
{
	if constexpr (std::is_same_v<T, wchar_t>)
		return str;
	else
		return ww898::utf::conv<wchar_t>(str);
}

#else

constexpr auto _To_bytes(const std::basic_string_view<char> str) noexcept
{
	return str;
}

constexpr auto _To_wide(const std::basic_string_view<char> str) noexcept
{
	return str;
}
#endif

