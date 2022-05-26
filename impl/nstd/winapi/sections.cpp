module;

#include <windows.h>
#include <winternl.h>

#include <string_view>

module nstd.winapi.sections;
import nstd.winapi.helpers;

IMAGE_SECTION_HEADER* nstd::winapi::find_section(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name)
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
