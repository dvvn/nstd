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
    IMAGE_SECTION_HEADER* find_section(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name) noexcept;

    template <typename Msg>
    IMAGE_SECTION_HEADER* find_section(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::wstring_view module_name, const std::string_view section_name) noexcept
    {
        const auto found = find_section(ldr_entry, section_name);
        _Invoke_msg<Msg>(found, module_name, section_name);
        return found;
    }

    template <text::chars_cache Module, text::chars_cache Section, typename Msg = void*>
    IMAGE_SECTION_HEADER* find_section() runtime_assert_noexcept
    {
        static const auto found = find_section<Msg>(find_module<Module, Msg>(), Module, Section);
        _Protect_args<Module, Section>(__FUNCSIG__);
        return found;
    }
} // namespace nstd::winapi

module :private;
import nstd.winapi.helpers;

IMAGE_SECTION_HEADER* nstd::winapi::find_section(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name) noexcept
{
    const auto [dos, nt] = dos_nt(ldr_entry);

    const auto number_of_sections = nt->FileHeader.NumberOfSections;
    const auto section_header = IMAGE_FIRST_SECTION(nt);
    const auto last_section_header = section_header + number_of_sections;

    for (auto header = section_header; header != last_section_header; ++header)
    {
        if (std::memcmp(header->Name, name.data(), name.size()) != 0)
            continue;
        if (header->Name[name.size()] != '\0')
            continue;
        return header;
    }

    return nullptr;
}
