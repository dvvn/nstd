#include "exports_storage.h"
#include "nstd/os/module info.h"

#include <Windows.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <ranges>

using nstd::os::exports_storage;
using namespace nstd::os::detail;

template < >
module_data_cache_fwd<nstd::address> nstd::os::detail::make_cache_fwd<nstd::address>( )
{
	return std::make_unique<module_data_cache<address>>( );
}

template < >
bool nstd::os::detail::cache_empty<nstd::address>(const module_data_cache<address>& cache)
{
	return cache.empty( );
}

template < >
void nstd::os::detail::cache_reserve<nstd::address>(module_data_cache<address>& cache, std::size_t count)
{
	cache.reserve(count);
}

bool exports_storage::load_from_memory(cache_type& cache)
{
	const auto nt = this->nt_header( );

	// get export data directory.
	const auto data_dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!data_dir->VirtualAddress)
		return false;

	const auto base_address = this->base_addr( );

	// get export dir.
	const auto dir = base_address.add(data_dir->VirtualAddress).ptr<IMAGE_EXPORT_DIRECTORY>( );
#ifdef NDEBUG
    if (!dir)
        return 0;
#endif
	// names / funcs / ordinals ( all of these are RVAs ).
	const auto names = base_address.add(dir->AddressOfNames).ptr<uint32_t>( );
	const auto funcs = base_address.add(dir->AddressOfFunctions).ptr<uint32_t>( );
	const auto ords  = base_address.add(dir->AddressOfNameOrdinals).ptr<uint16_t>( );
#ifdef NDEBUG
    if (!names || !funcs || !ords)
        return 0;
#endif

	const auto all_modules = all_modules::get_ptr( );
	all_modules->update(false);

	// iterate names array.
	for (auto i = 0u; i < dir->NumberOfNames; ++i)
	{
		const std::string_view export_name = base_address.add(names[i]).ptr<const char>( );
		if (export_name.empty( ) /*|| export_name.starts_with('?') || export_name.starts_with('@')*/)
			continue;

		/*
		 if (export_addr.cast<uintptr_t>() >= reinterpret_cast<uintptr_t>(export_directory)
			&& export_addr.cast<uintptr_t>() < reinterpret_cast<uintptr_t>(export_directory) + data_directory->Size)
		 */

		//if (export_ptr < dir || export_ptr >= memory_block(dir, data_dir->Size).addr( ))
		if (const auto export_ptr = base_address + funcs[ords[i]]; export_ptr < dir || export_ptr >= address(dir) + data_dir->Size)
		{
			cache.emplace(export_name, export_ptr);
		}
		else // it's a forwarded export, we must resolve it.
		{
			// get forwarder string.
			const std::string_view fwd_str = export_ptr.ptr<const char>( );

			// forwarders have a period as the delimiter.
			const auto delim = fwd_str.find_last_of('.');
			if (delim == fwd_str.npos)
				continue;

			const auto fwd_module_str = [&]
			{
				namespace rng = std::ranges;
				namespace rngv = rng::views;

				// get forwarder mod name.
				const auto name       = fwd_str.substr(0, delim);
				const auto name_lower = rngv::transform(name, [](const char c) { return static_cast<wchar_t>(std::tolower(c)); });

				return std::wstring(name_lower.begin( ), name_lower.end( )).append(L".dll");
			}( );

			// get forwarder export name.
			const auto fwd_export_str = fwd_str.substr(delim + 1);

			// get real export ptr ( recursively ).
			const auto target_module = all_modules->find(&module_info::name, fwd_module_str);
			if (!target_module)
				continue;

			auto& exports = target_module->exports( );
			if (!exports.load({ }))
			{
				runtime_assert("Unable to load exports!");
				continue;
			}

			const auto& exports_cache  = exports.get_cache( );
			const auto  fwd_export_ptr = exports_cache.find((fwd_export_str));
			if (fwd_export_ptr == exports_cache.end( ))
				continue;

			cache.emplace(export_name, fwd_export_ptr->second);
		}
	}

	//data_cache_.shrink_to_fit( );
	return true;
}

bool exports_storage::load_from_file(cache_type& cache, ptree_type&& storage)
{
	const auto base_address = this->base_addr( );

	for (auto itr = storage.begin( ); itr != storage.end( ); ++itr)
	{
		auto& name          = itr.key( );
		auto& offset_packed = itr.value( );

		const auto offset = offset_packed.get<uintptr_t>( );
		auto       addr   = base_address + offset;

		cache.emplace(const_cast<std::string&&>(name), std::move(addr));
	}

	return true;
}

bool exports_storage::read_to_storage(const cache_type& cache, ptree_type& storage) const
{
	const auto base_address = this->base_addr( );

	for (const auto& [name, addr]: cache)
	{
		storage[name] = addr.remove(base_address).value( );
	}

	return true;
}

//void exports_storage::Change_base_address_impl(address new_addr)
//{
//	for (auto itr = data_cache.begin( ); itr != data_cache.end( ); ++itr)
//	{
//		auto& val = itr.value( );
//		val -= (base_address);
//		val += new_addr;
//	}
//}
