module;

#include <nstd/winapi/msg_invoke.h>

#include <windows.h>
#include <winternl.h>

#include <string_view>
#include <concepts>

export module nstd.winapi.modules;
export import nstd.text.chars_cache;

#ifndef __cpp_lib_string_contains
#define contains(_X_) find(_X_) != static_cast<size_t>(-1)
#endif

using _Str = std::basic_string<WCHAR>;
using _Strv = std::basic_string_view<WCHAR>;

LDR_DATA_TABLE_ENTRY* find_module_impl(const _Strv name, const bool check_whole_path) noexcept;

export namespace nstd::winapi
{
	using ::_Str;
	using ::_Strv;

	template<typename Msg = void*>
	LDR_DATA_TABLE_ENTRY* find_module(const _Strv name) noexcept
	{
		const auto found = find_module_impl(name, name.contains(':'));
		_Invoke_msg<Msg>(found, name);
		return found;
	}

	template<text::chars_cache Name, typename Msg = void*>
	auto find_module( ) noexcept
	{
		static const auto found = find_module<Msg>(Name.view( ));
		return found;
	}

	LDR_DATA_TABLE_ENTRY* current_module( ) noexcept;
}