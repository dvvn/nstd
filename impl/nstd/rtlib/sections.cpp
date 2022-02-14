module;

#include "info_includes.h"

//#include "nstd/ranges.h"
//#include "nstd/file/to_memory.h"

#include <Windows.h>

module nstd.rtlib:sections;
import :info;

using namespace nstd;
using namespace rtlib;

auto sections_storage::create(const key_type& entry) -> create_result
{
	const auto nt = root_class( )->NT( );
	const auto base_address = root_class( )->base( );

	const auto number_of_sections = nt->FileHeader.NumberOfSections;
	cache::reserve(number_of_sections);

	const auto section_header = IMAGE_FIRST_SECTION(nt);
	const auto last_section_header = section_header + number_of_sections;

	for (auto header = section_header; header != last_section_header; ++header)
	{
		key_type info_name = (const char*)header->Name;
		mapped_type info;
		info.block = {base_address + header->VirtualAddress, header->SizeOfRawData};
		info.data = header;

		cache::emplace(std::move(info_name), std::move(info));
	}
}

