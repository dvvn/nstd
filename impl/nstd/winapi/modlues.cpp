module;

#include <nstd/runtime_assert.h>

#include <windows.h>
#include <winternl.h>

#include <string_view>
#include <functional>

module nstd.winapi.modules;
import nstd.winapi.module_info;
import nstd.mem.address;

using namespace nstd;

static auto _Get_ldr( ) noexcept
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
LDR_DATA_TABLE_ENTRY* _Find_module(const Fn comparer) noexcept
{
	const auto ldr = _Get_ldr( );
	// get module linked list.
	const auto list = std::addressof(ldr->InMemoryOrderModuleList);
	// iterate linked list.
	for (auto it = list->Flink; it != list; it = it->Flink)
	{
		// get current entry.
		const auto ldr_entry = CONTAINING_RECORD(it, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if (!ldr_entry)
			continue;

		//base address
		const basic_address<IMAGE_DOS_HEADER> _dos = ldr_entry->DllBase;
		// check for invalid DOS / DOS signature.
		if (!_dos || _dos->e_magic != IMAGE_DOS_SIGNATURE /* 'MZ' */)
			continue;

		const basic_address<IMAGE_NT_HEADERS> _nt = _dos + _dos->e_lfanew;
		// check for invalid NT / NT signature.
		if (!_nt || _nt->Signature != IMAGE_NT_SIGNATURE /* 'PE\0\0' */)
			continue;

		IMAGE_NT_HEADERS* const nt = _nt;
		IMAGE_DOS_HEADER* const dos = _dos;

		if (!std::invoke(comparer, ldr_entry, nt, dos))
			continue;

		return ldr_entry;
	}

	return nullptr;
}

LDR_DATA_TABLE_ENTRY* _Find_module(const _Strv name, const bool check_whole_path) noexcept
{
	return _Find_module([=](LDR_DATA_TABLE_ENTRY* const ldr_entry, IMAGE_NT_HEADERS*, IMAGE_DOS_HEADER*) noexcept
	{
		const auto full_path = winapi::module_info(ldr_entry).path( );
		if (check_whole_path)
			return full_path == name;
		if (!full_path.ends_with(name) || *std::next(full_path.rbegin( ), name.size( )) != '\\')
			return false;
		return true;
	});
}

LDR_DATA_TABLE_ENTRY* find_module_impl(const _Strv name, const bool check_whole_path) noexcept
{
	return _Find_module(name, check_whole_path);
}

static DECLSPEC_NOINLINE HMODULE _Get_current_module_handle( ) noexcept
{
	MEMORY_BASIC_INFORMATION info;
	constexpr SIZE_T info_size = sizeof(MEMORY_BASIC_INFORMATION);

	//todo: is this is dll, try to load this function from inside
	[[maybe_unused]]
	const auto len = VirtualQueryEx(GetCurrentProcess( ), _Get_current_module_handle, std::addressof(info), info_size);
	runtime_assert(len == info_size, "Wrong size");
	return static_cast<HMODULE>(info.AllocationBase);
}

LDR_DATA_TABLE_ENTRY* winapi::current_module( ) noexcept
{
	static const auto ret = _Find_module([base_address = _Get_current_module_handle( )](LDR_DATA_TABLE_ENTRY*, IMAGE_NT_HEADERS*, IMAGE_DOS_HEADER* const dos) noexcept
	{
		return base_address == reinterpret_cast<HMODULE>(dos);
	});
	return ret;
}

