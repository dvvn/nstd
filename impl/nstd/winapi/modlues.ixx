module;

#include <nstd/chars cache.h>
#include <windows.h>
#include <winternl.h>
#include <string_view>
//#include <functional>
#include <concepts>

export module nstd.winapi.modules;

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::string_view name, bool check_whole_path);
LDR_DATA_TABLE_ENTRY* find_module_impl(const std::wstring_view name, bool check_whole_path);

template<typename T>
constexpr bool check_whole_path(const std::basic_string_view<T> name)
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

export namespace nstd::winapi
{
	template<typename Msg, typename T>
	LDR_DATA_TABLE_ENTRY* find_module_impl(const std::basic_string_view<T> name)
	{
		const auto found = ::find_module_impl(name, ::check_whole_path(name));
		if constexpr (std::invocable<Msg, decltype(found), decltype(name)>)
		{
			Msg msg;
			msg(found, name);
		}
		return found;
	}

	template<chars_cache Name, typename Msg = void*>
	auto find_module( )
	{
		static auto found = find_module_impl<Msg>(Name.view( ));
		return found;
	}

	LDR_DATA_TABLE_ENTRY* current_module( );
}