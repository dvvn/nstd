module;

#include <nstd/chars cache.h>
#include <windows.h>
#include <winternl.h>
#include <string_view>
#include <functional>

export module nstd.winapi.sections;
export import nstd.winapi.modules;

export namespace nstd::winapi
{
	IMAGE_SECTION_HEADER* find_section_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::string_view name);

	template<typename Msg, typename T>
	IMAGE_SECTION_HEADER* find_section_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::basic_string_view<T> module_name, const std::string_view section_name)
	{
		const auto found = find_section_impl(ldr_entry, section_name);
		if constexpr (std::invocable<Msg, decltype(found), decltype(module_name), decltype(section_name)>)
		{
			Msg msg;
			msg(found, module_name, section_name);
		}
		return found;
	}

	template<chars_cache Module, chars_cache Section, typename Msg = void*>
	IMAGE_SECTION_HEADER* find_section( )
	{
		static auto found = find_section_impl<Msg>(find_module<Module, Msg>( ), Module.view( ), Section.view( ));
		return found;
	}
}
