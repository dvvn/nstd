#include "exports mgr.h"

#include "cache_impl.h"

#include <nstd/os/module info.h>

#include <Windows.h>

using namespace nstd;
using namespace nstd::os;

NSTD_OS_MODULE_INFO_CACHE_IMPL_CPP(exports_mgr, export_info)
{
	//fill whole cache

	const auto nt = module_info_ptr->NT( );

	// get export data directory.
	const auto data_dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!data_dir->VirtualAddress)
		throw std::runtime_error("Current module doesnt have virtual address!");

	const auto base_address = module_info_ptr->base( );

	// get export dir.
	const auto dir = base_address.add(data_dir->VirtualAddress).ptr<IMAGE_EXPORT_DIRECTORY>( );
#ifdef NDEBUG
    if (!dir)
		return {};
#endif
	// names / funcs / ordinals ( all of these are RVAs ).
	const auto names = base_address.add(dir->AddressOfNames).ptr<uint32_t>( );
	const auto funcs = base_address.add(dir->AddressOfFunctions).ptr<uint32_t>( );
	const auto ords  = base_address.add(dir->AddressOfNameOrdinals).ptr<uint16_t>( );
#ifdef NDEBUG
    if (!names || !funcs || !ords)
        return {};
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
			this->emplace(std::string(export_name), export_info(export_ptr));
			//
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
				// get forwarder mod name.
				const auto fwd_module_name       = fwd_str.substr(0, delim);
				const auto fwd_module_name_lower = std::views::transform(fwd_module_name, [](const char c) { return static_cast<wchar_t>(std::tolower(c)); });

				return std::wstring(fwd_module_name_lower.begin( ), fwd_module_name_lower.end( )).append(L".dll");
			}( );

			// get forwarder export name.
			const auto fwd_export_str = fwd_str.substr(delim + 1);

			// get real export ptr ( recursively ).
			const auto target_module = all_modules->find([&](const module_info& info) { return info.name( ) == fwd_module_str; });
			if (!target_module)
				continue;
			try
			{
				auto& exports       = target_module->exports( );
				auto fwd_export_ptr = exports.at(fwd_export_str);

				this->emplace(std::string(export_name), std::move(fwd_export_ptr));
			}
			catch (std::exception)
			{
			}
		}
	}

	return {{}, false};
}
