module;
#include "info_includes.h"
#include "nstd/ranges.h"

module nstd.rtlib:exports;
import :all_infos;
import nstd.container.wrapper;
import nstd.text.actions;

using namespace nstd;
using namespace rtlib;

auto exports_storage::create(const key_type& entry) -> create_result
{
	//fill whole cache

	const auto nt = root_class( )->NT( );

	// get export data directory.
	const auto& data_dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!data_dir.VirtualAddress)
		throw std::runtime_error("Current module doesn't have virtual address!");

	const basic_address base_address = root_class( )->DOS( );

	// get export export_dir.
	const basic_address<IMAGE_EXPORT_DIRECTORY> export_dir = base_address + data_dir.VirtualAddress;
	//#ifdef NDEBUG
	//	if (!export_dir)
	//		return;
	//#endif
		// names / funcs / ordinals ( all of these are RVAs ).
	uint32_t* const  names = base_address + export_dir->AddressOfNames;
	uint32_t* const  funcs = base_address + export_dir->AddressOfFunctions;
	uint16_t* const  ords = base_address + export_dir->AddressOfNameOrdinals;
	//#ifdef NDEBUG
	//	if (!names || !funcs || !ords)
	//		return;
	//#endif

	const auto all_modules = all_infos::get_ptr( );
	all_modules->update(false);

	// iterate names array.
	for (auto i = 0u; i < export_dir->NumberOfNames; ++i)
	{
		const char* export_name = base_address + names[i];
		if (!export_name || *export_name == '\0'/*export_name.empty( ) || export_name.starts_with('?') || export_name.starts_with('@')*/)
			continue;

		/*
		 if (export_addr.cast<uintptr_t>() >= reinterpret_cast<uintptr_t>(export_directory)
			&& export_addr.cast<uintptr_t>() < reinterpret_cast<uintptr_t>(export_directory) + data_directory->Size)
		 */

		 //if (export_ptr < export_dir || export_ptr >= memory_block(export_dir, data_dir.Size).addr( ))
		const auto export_ptr = base_address + funcs[ords[i]];
		if (export_ptr < export_dir || export_ptr >= export_dir + data_dir.Size)
		{
			this->emplace(export_name, export_ptr);
			//
		}
		else // it's a forwarded export, we must resolve it.
		{
			// get forwarder string.
			const std::string_view fwd_str = export_ptr.get<const char*>( );

			// forwarders have a period as the delimiter.
			const auto delim = fwd_str.find_last_of('.');
			if (delim == fwd_str.npos)
				continue;

			using namespace std::string_view_literals;
			// get forwarder mod name.
			const info_string::fixed_type fwd_module_str = nstd::append<std::wstring>(fwd_str.substr(0, delim), L".dll"sv);

			// get real export ptr ( recursively ).
			const auto target_module = std::ranges::find_if(*all_modules, [&](const info& i)
			{
				return i.name == fwd_module_str;
			});
			if (target_module == all_modules->end( ))
				continue;

			// get forwarder export name.
			const auto fwd_export_str = fwd_str.substr(delim + 1);

			try
			{
				auto& exports = target_module->exports( );
				auto fwd_export_ptr = exports.at(fwd_export_str);

				this->emplace(export_name, fwd_export_ptr);
			}
			catch (std::exception)
			{
			}
		}
	}
}
