module;

#include <string>

export module nstd.text.convert;
export import nstd.text.string;

export namespace nstd::inline text
{
	nstd::string to_lower(const char* str);
	nstd::string to_lower(const std::string_view str);

	nstd::wstring to_lower(const wchar_t* str);
	nstd::wstring to_lower(const std::wstring_view str);
}