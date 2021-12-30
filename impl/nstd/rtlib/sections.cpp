module;

#include "info_includes.h"

#include "nstd/ranges.h"
#include "nstd/file/to_memory.h"

#include <Windows.h>

module nstd.rtlib.info:sections;

using namespace nstd::rtlib;

auto sections::create(const key_type& entry) -> create_result
{
	const auto nt = root_class( )->NT( );
	const auto base_address = root_class( )->base( );

	const auto number_of_sections = nt->FileHeader.NumberOfSections;
	this->reserve(number_of_sections);

	const auto section_header = IMAGE_FIRST_SECTION(nt);
	const auto last_section_header = section_header + number_of_sections;

	for (auto header = section_header; header != last_section_header; ++header)
	{
		auto& raw_name = header->Name;
		auto info_name = key_type(raw_name, std::ranges::find(raw_name, '\0'));

		mapped_type info;
		info.block = {base_address + header->VirtualAddress, header->SizeOfRawData};
		info.data = header;

		this->emplace(std::move(info_name), std::move(info));
	}

	return create_result{{}, false};
}
