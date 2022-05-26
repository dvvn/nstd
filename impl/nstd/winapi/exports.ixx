module;

#include "internal/arguments_protect.h"
#include "internal/msg_invoke.h"

#include <windows.h>
#include <winternl.h>

#include <string_view>

export module nstd.winapi.exports;
export import nstd.winapi.modules;

export namespace nstd::winapi
{
    void* find_export(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name);

    template <typename FnT>
    struct found_export
    {
        FnT pointer;

        found_export(void* const ptr) : pointer(static_cast<FnT>(ptr))
        {
        }
    };

    template <typename FnT = void*, typename Msg = void*>
    FnT find_export(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::wstring_view module_name, const std::string_view export_name)
    {
        const auto found = find_export(ldr_entry, export_name);
        _Invoke_msg<Msg, found_export<FnT>>(found, module_name, export_name);
        return static_cast<FnT>(found);
    }

    template <typename FnT, text::chars_cache Module, text::chars_cache Export, typename Msg = void*>
    FnT find_export()
    {
        static const auto found = find_export<FnT, Msg>(find_module<Module, Msg>(), Module, Export);
        _Protect_args<Module, Export>(__FUNCSIG__);
        return found;
    }
} // namespace nstd::winapi
