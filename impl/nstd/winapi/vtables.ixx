module;

#include <nstd/winapi/msg_invoke.h>

#include <windows.h>
#include <winternl.h>

#include <string_view>
#include <functional>

export module nstd.winapi.vtables;
export import nstd.winapi.modules;

export namespace nstd::winapi
{
	void* find_vtable(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name) noexcept;

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

	template<typename Msg = void*>
	void* find_vtable(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::wstring_view module_name, const std::string_view vtable_name) noexcept
	{
		const auto found = find_vtable(ldr_entry, vtable_name);
		_Invoke_msg<Msg, found_vtable<void*>>(found, module_name, vtable_name);
		return found;
	}

	template<typename T, text::chars_cache Module, text::chars_cache Class, typename Msg = void*>
	T* find_vtable() noexcept
	{
		static const found_vtable<T> found = find_vtable<Msg>(find_module<Module, Msg>(), Module, Class);
		return found.ptr;
	}
}