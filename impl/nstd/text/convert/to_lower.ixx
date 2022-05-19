module;

#include <string>

export module nstd.text.convert.to_lower;

struct to_lower_obj
{
	std::string operator()(const char* str) const noexcept;
	std::string operator()(const std::string_view str) const noexcept;

	std::wstring operator()(const wchar_t* str) const noexcept;
	std::wstring operator()(const std::wstring_view str) const noexcept;
};

export namespace nstd::text
{
    constexpr to_lower_obj to_lower;
}
