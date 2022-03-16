module;

#include <nstd/chars cache.h>
#include <windows.h>
#include <winternl.h>
#include <string_view>
#include <functional>

export module nstd.winapi.exports;
export import nstd.winapi.modules;

void* find_export_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::string_view name);

export namespace nstd::winapi
{
	template<typename Fn>
	struct found_export
	{
		union
		{
			Fn known;
			void* unknown;
		};

		found_export(void* ptr)
			:unknown(ptr)
		{
		}
	};

	template<typename Msg, typename T>
	void* find_export_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::basic_string_view<T> module_name, const std::string_view export_name)
	{
		const auto found = ::find_export_impl(ldr_entry, export_name);
		if constexpr (std::invocable<Msg, found_export<void*>, std::basic_string_view<T>, std::string_view>)
		{
			Msg msg;
			msg(found_export<void*>(found), module_name, export_name);
		}
		return found;
	}

	template<typename Fn, chars_cache Module, chars_cache Export, typename Msg = void*>
	Fn find_export( )
	{
		/*static found_export<Fn> found = find_export(find_module<Module, Msg>( ), Export.view( ));
		if constexpr (std::invocable<Msg, decltype(found), decltype(Module.view( )), decltype(Export.view( ))>)
			static const uint8_t once = (std::invoke(Msg( ), found, Module.view( ), Export.view( )), 0);
		return found.known;*/
		static found_export<Fn> found = find_export_impl<Msg>(find_module<Module, Msg>( ), Module.view( ), Export.view( ));
		return found.known;
	}
}