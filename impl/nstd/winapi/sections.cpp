module;

#include <nstd/runtime_assert.h>
#include <windows.h>
#include <winternl.h>
#include <string_view>

module nstd.winapi.sections;
import nstd.mem.address;

using namespace nstd::mem;

IMAGE_SECTION_HEADER* nstd::winapi::find_section_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::string_view name)
{
	//base address
	const basic_address<IMAGE_DOS_HEADER> dos = ldr_entry->DllBase;
	const basic_address<IMAGE_NT_HEADERS> nt = dos + dos->e_lfanew;

	const auto number_of_sections = nt->FileHeader.NumberOfSections;
	const auto section_header = IMAGE_FIRST_SECTION(nt);
	const auto last_section_header = section_header + number_of_sections;

	for (auto header = section_header; header != last_section_header; ++header)
	{
		if (std::memcmp(header->Name, name.data( ), name.size( )) != 0)
			continue;
		if (header->Name[name.size( )] != '\0')
			continue;

		//info.block = {dos + header->VirtualAddress, header->SizeOfRawData};
		//info.data = header;

		return header;
	}

	return nullptr;
}