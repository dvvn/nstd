module;
#include "nstd/runtime_assert.h"
#include "nstd/ranges.h"

#include <Windows.h>
#include <winternl.h>

#include <list>
#include <optional>
#include <vector>
#include <algorithm>
#include <functional>

module nstd.rtlib.all_infos;
import nstd.address;

using namespace nstd;
using namespace nstd::rtlib;

struct header
{
	PIMAGE_DOS_HEADER dos;
	PIMAGE_NT_HEADERS nt;
};

static std::optional<header> _Get_file_headers(address base)
{
	const auto dos = base.ptr<IMAGE_DOS_HEADER>( );

	// check for invalid DOS / DOS signature.
	if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE /* 'MZ' */)
		return {};

	// get NT headers.
	const auto nt = address(dos).add(dos->e_lfanew).ptr<IMAGE_NT_HEADERS>( );

	// check for invalid NT / NT signature.
	if (!nt || nt->Signature != IMAGE_NT_SIGNATURE /* 'PE\0\0' */)
		return {};

	header result;

	// set out dos and nt.
	result.dos = dos;
	result.nt = nt;

	return result;
}

static auto _Get_all_modules( )
{
	// TEB->ProcessEnvironmentBlock.
#if defined(_M_X64) || defined(__x86_64__)
	const auto mem = NtCurrentTeb( );
	runtime_assert(mem != nullptr, "Teb not found");
#else
	const auto mem = reinterpret_cast<PPEB>(__readfsdword(0x30));
	runtime_assert(mem != nullptr, "Peb not found");
#endif

#if defined(_M_X64) || defined(__x86_64__)
	const auto ldr = mem->ProcessEnvironmentBlock->Ldr;
#else
	const auto ldr = mem->Ldr;
#endif

	auto container = std::vector<info>( );

	// get module linked list.
	const auto list = &ldr->InMemoryOrderModuleList;
	// iterate linked list.
	for (auto it = list->Flink; it != list; it = it->Flink)
	{
		// get current entry.
		const auto ldr_entry = CONTAINING_RECORD(it, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if (!ldr_entry)
			continue;

		// get file headers and ensure it's a valid PE file.
		const auto headers = _Get_file_headers(ldr_entry->DllBase);
		if (!headers.has_value( ))
			continue;

		// push module to out container.
		container.emplace_back(ldr_entry, headers->dos, headers->nt);
	}

	//all internal functions tested only on x86
	//UPDATE: all works, except vtables finder

	runtime_assert(std::ranges::adjacent_find(container, {}, &info::name) == container.end( ));
	return container;
}

//module_info_loaded::module_info_loaded(const boost::filesystem::path& p, DWORD flags):
//    handle_(LoadLibraryExW(p.c_str( ), nullptr, flags))
//{
//    runtime_assert(handle_!=nullptr, "Unable to load module");
//
//
//    auto all_infos=get_all_modules()
//
//}

static volatile DECLSPEC_NOINLINE HMODULE _Get_current_module_handle( )
{
	MEMORY_BASIC_INFORMATION info;
	constexpr SIZE_T info_size = sizeof(MEMORY_BASIC_INFORMATION);

	//todo: is this is dll, try to load this function from inside
	const auto len = VirtualQueryEx(GetCurrentProcess( ), _Get_current_module_handle, &info, info_size);
	runtime_assert(len == info_size, "Wrong size");
	return static_cast<HMODULE>(info.AllocationBase);
}

info& modules_storage::current( ) const
{
	return *current_cached_;
}

modules_storage& modules_storage::update(bool force)
{
	static const address current_base = _Get_current_module_handle( );

	if (this->empty( ))
	{
		runtime_assert(current_cached_ == nullptr, "Cache already set");
		
		for (auto& m : _Get_all_modules( ))
		{
			auto& item = this->emplace_back(std::move(m));
			if (current_cached_ == nullptr && item.base( ) == current_base)
				current_cached_ = std::addressof(item);
		}
	}
	else if (force)
	{
		auto all = _Get_all_modules( );

		//erase all unused modules
		this-remove_if([&](const info& m)-> bool
							{
								for (const auto& m_new : all)
								{
									if (m.base( ) == m_new.base( ))
										return false;
								}
								return true;
							});

		auto find_current = current_cached_ != nullptr;

		//add all new modules, save the order
		auto itr = this->begin( );
		for (auto& m : all)
		{
			if (itr == this->end( ) || itr->base( ) != m.base( ))
				itr = this->insert(itr, const_cast<info&&>(m));

			if (find_current && itr->base( ) == current_base)
			{
				current_cached_ = std::addressof(*itr);
				find_current = false;
			}

			++itr;
		}
	}

	return *this;
}

info& modules_storage::owner( )
{
	return this->front( );
}

template <typename T, typename Pred>
static info* _Find(T&& storage, Pred&& pred)
{
	for (info& i : storage)
	{
		if (std::invoke(pred, i))
			return std::addressof(i);
	}

	return nullptr;
}

info* modules_storage::find(const find_fn& fn)
{
	return _Find(*this, fn);
}

info* modules_storage::rfind(const find_fn& fn)
{
	return _Find(*this | std::views::reverse, fn);
}
