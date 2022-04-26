module;

#include <nstd/winapi/msg_invoke.h>

#include <windows.h>
#include <winternl.h>

#include <string_view>
#include <functional>

export module nstd.winapi.exports;
export import nstd.winapi.modules;

export namespace nstd::winapi
{
	void* find_export(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name) noexcept;

	template<typename FnT>
	struct found_export
	{
		union
		{
			FnT known;
			void* unknown;
		};

		found_export(void* ptr)
			:unknown(ptr)
		{
		}
	};

	template<typename Msg = void*>
	void* find_export(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::wstring_view module_name, const std::string_view export_name) noexcept
	{
		const auto found = find_export(ldr_entry, export_name);
		_Invoke_msg<Msg, found_export<void*>>(found, module_name, export_name);
		return found;
	}

	template<typename FnT, text::chars_cache Module, text::chars_cache Export, typename Msg = void*>
	FnT find_export() noexcept
	{
		static const found_export<FnT> found = find_export<Msg>(find_module<Module, Msg>(), Module, Export);
		return found.known;
	}
}