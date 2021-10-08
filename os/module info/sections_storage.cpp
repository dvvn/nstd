#include "sections_storage.h"

#include <Windows.h>

#include <nlohmann/json.hpp>

using nstd::os::sections_storage;
using nstd::os::section_info;
using namespace nstd::os::detail;

template < >
module_data_cache_fwd<section_info> nstd::os::detail::make_cache_fwd<section_info>( )
{
	return std::make_unique<module_data_cache<section_info>>( );
}

template < >
bool nstd::os::detail::cache_empty<section_info>(const module_data_cache<section_info>& cache)
{
	return cache.empty( );
}

template < >
void nstd::os::detail::cache_reserve<section_info>(module_data_cache<section_info>& cache, std::size_t count)
{
	cache.reserve(count);
}

bool sections_storage::load_from_memory(cache_type& cache)
{
	const auto nt           = this->nt_header( );
	const auto base_address = this->base_addr( );

	const auto number_of_sections = nt->FileHeader.NumberOfSections;
	cache.reserve(number_of_sections);

	const auto section_header      = IMAGE_FIRST_SECTION(nt);
	const auto last_section_header = section_header + number_of_sections;

	for (auto header = section_header; header != last_section_header; ++header)
	{
		auto& raw_name  = header->Name;
		auto  info_name = std::string(raw_name, std::ranges::find(raw_name, '\0'));

		section_info info;
		info.block = {base_address + header->VirtualAddress, header->SizeOfRawData};
		info.data  = header;

		cache.emplace(std::move(info_name), std::move(info));
	}

	return true;
}

bool sections_storage::load_from_file(cache_type& cache, detail::ptree_type&& storage)
{
	return false;
}

bool sections_storage::read_to_storage(const cache_type& cache, detail::ptree_type& storage) const
{
	return false;
}

#if 0
void sections_storage::Change_base_address_impl(address new_addr)
{
	const auto base_address = this->base_addr( );

	for (auto itr = data_cache.begin( ); itr != data_cache.end( ); ++itr)
	{
		// ReSharper disable once CppUseStructuredBinding
		auto& info = itr.value( );

		address header_addr = info.data;
		header_addr -= (base_address);
		header_addr += new_addr;
		const auto header = header_addr.ptr<IMAGE_SECTION_HEADER>( );

		info.block = {new_addr + header->VirtualAddress, header->SizeOfRawData};
		info.data = header;
	}
}
#endif
