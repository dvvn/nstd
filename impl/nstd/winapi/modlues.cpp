module;

#include <nstd/runtime_assert.h>
#include <windows.h>
#include <winternl.h>
#include <string_view>
#include <functional>

module nstd.winapi.modules;
import nstd.mem.address;

using namespace nstd::mem;

static auto _Get_ldr( )
{
	const auto mem =
#if defined(_M_X64) || defined(__x86_64__)
		NtCurrentTeb( );
	runtime_assert(mem != nullptr, "Teb not found");
	return mem->ProcessEnvironmentBlock->Ldr;
#else
		reinterpret_cast<PEB*>(__readfsdword(0x30));
	runtime_assert(mem != nullptr, "Peb not found");
	return mem->Ldr;
#endif
}

template<typename Fn>
static LDR_DATA_TABLE_ENTRY* _Find_module(const Fn comparer)
{
	const auto ldr = _Get_ldr( );
	// get module linked list.
	const auto list = &ldr->InMemoryOrderModuleList;
	// iterate linked list.
	for (auto it = list->Flink; it != list; it = it->Flink)
	{
		// get current entry.
		const auto ldr_entry = CONTAINING_RECORD(it, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if (!ldr_entry)
			continue;

		//base address
		const basic_address<IMAGE_DOS_HEADER> dos = ldr_entry->DllBase;
		// check for invalid DOS / DOS signature.
		if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE /* 'MZ' */)
			continue;

		const basic_address<IMAGE_NT_HEADERS> nt = dos + dos->e_lfanew;
		// check for invalid NT / NT signature.
		if (!nt || nt->Signature != IMAGE_NT_SIGNATURE /* 'PE\0\0' */)
			continue;

		if (!std::invoke(comparer, ldr_entry, nt, dos))
			continue;

		return ldr_entry;
	}

	return nullptr;
}

static constexpr auto _Module_name_comparer = []<typename ...Unused>(const std::basic_string_view<WCHAR> name, bool check_whole_path, LDR_DATA_TABLE_ENTRY * ldr_entry, Unused...)
{
	const std::basic_string_view<WCHAR> full_path = {ldr_entry->FullDllName.Buffer, ldr_entry->FullDllName.Length / sizeof(WCHAR)};
	if (check_whole_path && full_path != name)
		return false;
	if (!full_path.ends_with(name) || *std::next(full_path.rbegin( ), name.size( )) != '\\')
		return false;

	return true;
};

template<typename T>
LDR_DATA_TABLE_ENTRY* _Find_module_wide(const std::basic_string_view<T> name, bool check_whole_path)
{
	if constexpr (sizeof(T) == sizeof(WCHAR))
	{
		const std::basic_string_view buff = {reinterpret_cast<const WCHAR*>(name.data( )), name.size( )};
		return _Find_module(std::bind_front(_Module_name_comparer, buff, check_whole_path));
	}
	else if constexpr (sizeof(T) < sizeof(WCHAR))
	{
		const std::basic_string<WCHAR> buff = {name.begin( ), name.end( )};
		const std::basic_string_view<WCHAR> buff2 = buff;
		return _Find_module(std::bind_front(_Module_name_comparer, buff2, check_whole_path));
	}
	else
	{
		static_assert(false, __FUNCSIG__": Not implemented");
	}
}

//block = {(uint8_t*)DOS( ), NT( )->OptionalHeader.SizeOfImage};

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::wstring_view name, bool check_whole_path)
{
	return _Find_module_wide(name, check_whole_path);
}

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::string_view name, bool check_whole_path)
{
	return _Find_module_wide(name, check_whole_path);
}

static DECLSPEC_NOINLINE HMODULE _Get_current_module_handle( )
{
	MEMORY_BASIC_INFORMATION info;
	constexpr SIZE_T info_size = sizeof(MEMORY_BASIC_INFORMATION);

	//todo: is this is dll, try to load this function from inside
	const auto len = VirtualQueryEx(GetCurrentProcess( ), _Get_current_module_handle, std::addressof(info), info_size);
	runtime_assert(len == info_size, "Wrong size");
	return static_cast<HMODULE>(info.AllocationBase);
}

static basic_address<IMAGE_DOS_HEADER> _Get_current_dos_header( )
{
	return _Get_current_module_handle( );
}

LDR_DATA_TABLE_ENTRY* nstd::winapi::current_module( )
{
	static auto ret = _Find_module([base_address = _Get_current_dos_header( )](LDR_DATA_TABLE_ENTRY*, IMAGE_NT_HEADERS*, IMAGE_DOS_HEADER* dos)
	{
		return base_address == dos;
	});
	return ret;
}