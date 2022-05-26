module;

#include "internal/arguments_protect.h"
#include "internal/msg_invoke.h"

#include <windows.h>
#include <winternl.h>

#include <string_view>

export module nstd.winapi.sections;
export import nstd.winapi.modules;

export namespace nstd::winapi
{
    IMAGE_SECTION_HEADER* find_section(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name);

    template <typename Msg>
    IMAGE_SECTION_HEADER* find_section(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::wstring_view module_name, const std::string_view section_name)
    {
        const auto found = find_section(ldr_entry, section_name);
        _Invoke_msg<Msg>(found, module_name, section_name);
        return found;
    }

    template <text::chars_cache Module, text::chars_cache Section, typename Msg = void*>
    IMAGE_SECTION_HEADER* find_section()
    {
        static const auto found = find_section<Msg>(find_module<Module, Msg>(), Module, Section);
        _Protect_args<Module, Section>(__FUNCSIG__);
        return found;
    }
} // namespace nstd::winapi
