module;

#include "internal/arguments_protect.h"
#include "internal/msg_invoke.h"

#include <windows.h>
#include <winternl.h>

#include <functional>
#include <string_view>

export module nstd.winapi.exports;
export import nstd.winapi.modules;

export namespace nstd::winapi
{
    void* find_export(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name);

    template <typename FnT>
    struct found_export
    {
        FnT pointer;

        found_export(void* const ptr) : pointer(static_cast<FnT>(ptr))
        {
        }
    };

    template <typename FnT = void*, typename Msg = void*>
    FnT find_export(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::wstring_view module_name, const std::string_view export_name)
    {
        const auto found = find_export(ldr_entry, export_name);
        _Invoke_msg<Msg, found_export<FnT>>(found, module_name, export_name);
        return static_cast<FnT>(found);
    }

    template <typename FnT, text::chars_cache Module, text::chars_cache Export, typename Msg = void*>
    FnT find_export() runtime_assert_noexcept
    {
        static const auto found = find_export<FnT, Msg>(find_module<Module, Msg>(), Module, Export);
        _Protect_args<Module, Export>(__FUNCSIG__);
        return found;
    }
} // namespace nstd::winapi

module :private;
import nstd.mem.address;

void* nstd::winapi::find_export(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name)
{
    using mem::basic_address;

    // base address
    const basic_address<IMAGE_DOS_HEADER> dos = ldr_entry->DllBase;
    const basic_address<IMAGE_NT_HEADERS> nt = dos + dos->e_lfanew;

    // get export data directory.
    const auto& data_dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    runtime_assert(data_dir.VirtualAddress, "Current module doesn't have the virtual address!");

    // get export export_dir.
    const basic_address<IMAGE_EXPORT_DIRECTORY> export_dir = dos + data_dir.VirtualAddress;

    // names / funcs / ordinals ( all of these are RVAs ).
    uint32_t* const names = dos + export_dir->AddressOfNames;
    uint32_t* const funcs = dos + export_dir->AddressOfFunctions;
    uint16_t* const ords = dos + export_dir->AddressOfNameOrdinals;

    // iterate names array.
    for (auto i = 0u; i < export_dir->NumberOfNames; ++i)
    {
        const char* export_name = dos + names[i];
        if (!export_name)
            continue;
        if (std::memcmp(export_name, name.data(), name.size()) != 0)
            continue;
        if (export_name[name.size()] != '\0')
            continue;

        /*
         if (export_addr.cast<uintptr_t>() >= reinterpret_cast<uintptr_t>(export_directory)
            && export_addr.cast<uintptr_t>() < reinterpret_cast<uintptr_t>(export_directory) + data_directory->Size)
         */

        // if (export_ptr < export_dir || export_ptr >= memory_block(export_dir, data_dir.Size).addr( ))
        const auto export_ptr = dos + funcs[ords[i]];
        if (export_ptr < export_dir || export_ptr >= export_dir + data_dir.Size)
            return export_ptr;

        runtime_assert("Forwarded export detected");
        break;
#if 0
		// get forwarder string.
		const std::string_view fwd_str = export_ptr.get<const char*>( );

		// forwarders have a period as the delimiter.
		const auto delim = fwd_str.find_last_of('.');
		if(delim == fwd_str.npos)
			continue;

		using namespace std::string_view_literals;
		// get forwarder mod name.
		const info_string::fixed_type fwd_module_str = nstd::append<std::wstring>(fwd_str.substr(0, delim), L".dll"sv);

		// get real export ptr ( recursively ).
		const auto target_module = std::ranges::find_if(*all_modules, [&](const info& i)
		{
			return i.name == fwd_module_str;
		});
		if(target_module == all_modules->end( ))
			continue;

		// get forwarder export name.
		const auto fwd_export_str = fwd_str.substr(delim + 1);

		try
		{
			auto& exports = target_module->exports( );
			auto fwd_export_ptr = exports.at(fwd_export_str);

			this->emplace(export_name, fwd_export_ptr);
		}
		catch(std::exception)
		{
		}
#endif
    }

    return nullptr;
}
