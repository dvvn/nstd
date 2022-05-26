module;

#include "internal/arguments_protect.h"
#include "internal/msg_invoke.h"

#include <windows.h>
#include <winternl.h>

#include <string_view>

export module nstd.winapi.modules;
export import nstd.text.chars_cache;

#ifndef __cpp_lib_string_contains
#define contains(_X_) find(_X_) != std::wstring_view::npos
#endif

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::wstring_view name, const bool check_whole_path);

export namespace nstd::winapi
{
    template <typename Msg = void*>
    [[deprecated]] LDR_DATA_TABLE_ENTRY* find_module(const std::wstring_view name)
    {
        const auto found = find_module_impl(name, name.contains(':'));
        _Invoke_msg<Msg>(found, name);
        return found;
    }

    template <text::chars_cache Name, typename Msg = void*>
    auto find_module()
    {
        static const auto found = [] {
            constexpr std::wstring_view name = Name;
            _Protect_args<Name>(__FUNCSIG__);
            constexpr bool check_whole_path = name.contains(':');
            const auto ret = find_module_impl(name, check_whole_path);
            _Invoke_msg<Msg>(ret, name);
            return ret;
        }();

        return found;
    }

    LDR_DATA_TABLE_ENTRY* current_module();
} // namespace nstd::winapi
