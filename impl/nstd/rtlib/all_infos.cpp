module;

#include "includes.h"

module nstd.rtlib:all_infos;
import :info;
import nstd.mem;

using namespace nstd;
using namespace mem;
using namespace rtlib;

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
	auto
		const auto ldr = mem->ProcessEnvironmentBlock->Ldr;
#else
	const auto ldr = mem->Ldr;
#endif	

	std::vector<basic_info> container;

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

	//runtime_assert(std::ranges::adjacent_find(container, {}, &info::name) == container.end( ));
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
	const auto len = VirtualQueryEx(GetCurrentProcess( ), _Get_current_module_handle, std::addressof(info), info_size);
	runtime_assert(len == info_size, "Wrong size");
	return static_cast<HMODULE>(info.AllocationBase);
}

template<typename Rng>
static size_t _Get_current_index(const Rng& rng)
{
	static const address current_base = _Get_current_module_handle( );
	return std::distance(rng.begin( ), std::ranges::find(rng, current_base, &basic_info::base));
}

bool modules_storage::update(bool force)
{
	if (this->empty( ))
	{
		const auto all = _Get_all_modules( );
		this->reserve(all.size( ));
		for (auto& i : all)
			this->push_back(i);
		current_index_ = _Get_current_index(all);
		return true;
	}
	else if (force)
	{
		const auto all = _Get_all_modules( );
		if (std::ranges::equal(*this, all, [](const info& i, const basic_info& bi) {return i.base( ) == bi.base( ); }))
			return false;

		modules_storage_data storage;
		storage.reserve(all.size( ));

		if (this->empty( ))
		{
			for (const auto& i : all)
				storage.push_back(i);
		}
		else
		{
			for (auto& i : *this)
			{
				const auto itr = std::ranges::find(all, i.base( ), &basic_info::base);
				if (itr != all.end( ))
					storage[std::distance(itr, all.end( ))] = std::move(i);
			}

			for (size_t idx = 0; idx < all.size( ); ++idx)
			{
				static const basic_info def_val = {};
				auto& to = storage[idx];
				//already filled
				if (std::memcmp(static_cast<basic_info*>(std::addressof(to)), std::addressof(def_val), sizeof(basic_info)) != 0)
					continue;

				to = all[idx];
			}
		}

		*static_cast<modules_storage_data*>(this) = std::move(storage);
		current_index_ = _Get_current_index(all);
		return true;
	}

	return false;
}

const info& modules_storage::current( ) const
{
	return (*this)[current_index_];
}

info& modules_storage::current( )
{
	return (*this)[current_index_];
}

const info& modules_storage::owner( )const
{
	return this->front( );
}

info& modules_storage::owner( )
{
	return this->front( );
}
