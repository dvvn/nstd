module;

#include <cctype>
#include <cwctype>
#include <string>

export module nstd.text.actions;

export namespace nstd::inline text
{
	inline constexpr auto to_lower = []<typename Chr>(const Chr c)
	{
		if constexpr (std::same_as<Chr, char>)
			return static_cast<Chr>(std::tolower(static_cast<uint8_t>(c)));
		else if constexpr (std::same_as<Chr, wchar_t>)
			return static_cast<Chr>(std::towlower(static_cast<std::wint_t>(c)));
	};

	template<typename T>
	inline constexpr auto cast_all = []<typename Chr>(const Chr c)//todo: move it outside
	{
		return static_cast<T>(c);
	};
}