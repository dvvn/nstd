#include "sections.h"
#include "cache_impl.h"

#include "nstd/module/info.h"

#include <Windows.h>

using namespace nstd::module;

#include <nstd/file/to_memory.h>

NSTD_OS_MODULE_INFO_CACHE_IMPL_CPP(section)
{
	const auto nt           = info_ptr->NT( );
	const auto base_address = info_ptr->base( );

	const auto number_of_sections = nt->FileHeader.NumberOfSections;
	this->reserve(number_of_sections);

	const auto section_header      = IMAGE_FIRST_SECTION(nt);
	const auto last_section_header = section_header + number_of_sections;

	for (auto header = section_header; header != last_section_header; ++header)
	{
		auto& raw_name = header->Name;
		auto info_name = key_type(raw_name, std::ranges::find(raw_name, '\0'));

		mapped_type info;
		info.block = {base_address + header->VirtualAddress, header->SizeOfRawData};
		info.data  = header;

		this->emplace(std::move(info_name), std::move(info));
	}

	return create_result{{}, false};
}
