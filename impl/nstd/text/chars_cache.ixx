module;

#include <array>
#include <string_view>

export module nstd.text.chars_cache;

template <typename Chr, size_t Size>
struct chars_cache_impl
{
	std::array<Chr, Size> _Data;

	constexpr chars_cache_impl(const Chr* str_source)
	{
		std::char_traits<Chr>::copy(_Data.data( ), str_source, Size);
	}

	constexpr std::basic_string_view<Chr> view( ) const noexcept
	{
		return {_Data.data( ), Size - 1};
	}
};

export namespace nstd::inline text
{
	//use it for as template parameter

	template <typename Chr,size_t Size>
	struct chars_cache;

#define PROVIDE_CHARS_CACHE(_TYPE_)\
	template <size_t Size>\
	struct chars_cache<_TYPE_, Size> : chars_cache_impl<_TYPE_, Size>\
	{\
		using chars_cache_impl<_TYPE_, Size>::chars_cache_impl;\
	};

#ifdef __cpp_lib_char8_t
	PROVIDE_CHARS_CACHE(char8_t);
#endif
	PROVIDE_CHARS_CACHE(char);
	PROVIDE_CHARS_CACHE(wchar_t);
	PROVIDE_CHARS_CACHE(char16_t);
	PROVIDE_CHARS_CACHE(char32_t);

	template <typename Chr,size_t Size>
	chars_cache(const Chr(&arr)[Size])->chars_cache<Chr, Size>;
}
