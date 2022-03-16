module;

#include <nstd/chars cache.h>
#include <windows.h>
#include <winternl.h>
#include <string_view>
#include <functional>

export module nstd.winapi.vtables;
export import nstd.winapi.modules;

void* find_vtable_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::string_view name);

export namespace nstd::winapi
{
	template<typename T>
	struct found_vtable
	{
		using value_type = T*;

		value_type ptr;

		found_vtable(void* ptr)
			:ptr(static_cast<value_type>(ptr))
		{
		}
	};

	template<typename Msg, typename T>
	void* find_vtable_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::basic_string_view<T> module_name, const std::string_view vtable_name)
	{
		const auto found = find_vtable_impl(ldr_entry, vtable_name);
		if constexpr (std::invocable<Msg, found_vtable<void*>, std::basic_string_view<T>, std::string_view>)
		{
			Msg msg;
			msg(found_vtable<void*>(found), module_name, vtable_name);
		}
		return found;
	}

	template<typename T, chars_cache Module, chars_cache Class, typename Msg = void*>
	T* find_vtable( )
	{
		static found_vtable<T> found = find_vtable_impl<Msg>(find_module<Module, Msg>( ), Module.view( ), Class.view( ));
		return found.ptr;
	}
}