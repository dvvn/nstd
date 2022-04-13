module;

#include <windows.h>
#include <winternl.h>

#include <string_view>
//#include <functional>
#include <concepts>

export module nstd.winapi.modules;
import nstd.text.chars_cache;

template<typename T>
constexpr bool check_whole_path(const std::basic_string_view<T> name) noexcept
{
	constexpr auto chr = static_cast<T>(':');
	return name.
#ifdef __cpp_lib_string_contains
		contains(chr)
#else
		find(chr) != name.npos
#endif
		;
}

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::string_view name, const bool check_whole_path) noexcept;
LDR_DATA_TABLE_ENTRY* find_module_impl(const std::wstring_view name, const bool check_whole_path) noexcept;

class current_module_data
{
public:
	using pointer = LDR_DATA_TABLE_ENTRY*;
	using reference = LDR_DATA_TABLE_ENTRY&;

	current_module_data(pointer const entry);

	pointer operator->( ) const noexcept;
	reference operator*( ) const noexcept;
	reference operator[](ptrdiff_t offset) const noexcept;

	//---

	operator std::wstring_view( ) const noexcept;
	operator std::string_view( ) const noexcept;

private:
	pointer entry_;
};

using module_name_str =
#ifdef _UNICODE
std::string
#else
std::string_view
#endif
;

export namespace nstd::winapi
{
	template<typename Msg = void*, typename T>
	LDR_DATA_TABLE_ENTRY* find_module(const std::basic_string_view<T> name) noexcept
	{
		const auto found = find_module_impl(name, check_whole_path(name));
		if constexpr (std::invocable<Msg, decltype(found), decltype(name)>)
		{
			Msg msg;
			msg(found, name);
		}
		return found;
	}

	template<text::chars_cache Name, typename Msg = void*>
	auto find_module( ) noexcept
	{
		static const auto found = find_module<Msg>(Name.view( ));
		return found;
	}

	current_module_data current_module( ) noexcept;
	module_name_str get_module_name(LDR_DATA_TABLE_ENTRY* const ldr_entry) noexcept;
}