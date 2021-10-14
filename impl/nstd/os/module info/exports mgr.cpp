#include "exports mgr.h"

#include <nstd/runtime_assert_fwd.h>
#include <nstd/os/module info.h>

#include NSTD_OS_MODULE_INFO_DATA_CACHE_INCLUDE

#include <Windows.h>

#include <mutex>

using namespace nstd::os;

struct exports_mgr::storage_type : NSTD_OS_MODULE_INFO_DATA_CACHE<std::string, export_info>
{
	storage_type(const storage_type&)            = delete;
	storage_type& operator=(const storage_type&) = delete;

	storage_type(storage_type&&)            = default;
	storage_type& operator=(storage_type&&) = default;

	storage_type() = default;
};

exports_mgr::exports_mgr()
{
	storage_ = std::make_unique<storage_type>( );
}

exports_mgr::~exports_mgr()                                 = default;
exports_mgr::exports_mgr(exports_mgr&&) noexcept            = default;
exports_mgr& exports_mgr::operator=(exports_mgr&&) noexcept = default;

export_info exports_mgr::at(const std::string_view& name) const
{
	if (storage_->empty( ))
	{
		const auto lock = std::scoped_lock<const root_class_getter>(*this);

		//check can another thread do all the wordk
		if (storage_->empty( ))
		{
			const auto nt = this->root_class( )->NT( );

			// get export data directory.
			const auto data_dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			if (!data_dir->VirtualAddress)
				throw std::runtime_error("Current module doesnt have virtual address!");

			const auto base_address = this->root_class( )->base( );

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

			storage_type temp_storage;

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
					temp_storage.emplace(std::string(export_name), export_info(export_ptr));
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
						auto fwd_export_ptr = exports.at((fwd_export_str));

						temp_storage.emplace(std::string(export_name), (fwd_export_ptr));
					}
					catch (std::system_error)
					{
					}
				}
			}

			*storage_ = std::move(temp_storage);
		}
	}

	const auto el = storage_->find(std::string(name));
	runtime_assert(el != storage_->end( ));
	return el->second;
}
