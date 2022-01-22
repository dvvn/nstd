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

static auto _Get_ldr( )
{
	// TEB->ProcessEnvironmentBlock.
#if defined(_M_X64) || defined(__x86_64__)
	const auto mem = NtCurrentTeb( );
	runtime_assert(mem != nullptr, "Teb not found");
	return mem->ProcessEnvironmentBlock->Ldr;
#else
	const auto mem = reinterpret_cast<PPEB>(__readfsdword(0x30));
	runtime_assert(mem != nullptr, "Peb not found");
	return mem->Ldr;
#endif
}

template<typename T = basic_info>
static auto _Get_all_modules( )
{
	const auto ldr = _Get_ldr( );
	std::vector<T> container;
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

	return container;
}

static volatile DECLSPEC_NOINLINE HMODULE _Get_current_module_handle( )
{
	MEMORY_BASIC_INFORMATION info;
	constexpr SIZE_T info_size = sizeof(MEMORY_BASIC_INFORMATION);

	//todo: is this is dll, try to load this function from inside
	const auto len = VirtualQueryEx(GetCurrentProcess( ), _Get_current_module_handle, std::addressof(info), info_size);
	runtime_assert(len == info_size, "Wrong size");
	return static_cast<HMODULE>(info.AllocationBase);
}

static void _Write_current_idx(const modules_storage_data& storage, size_t& current_holder)
{
	const address current_base = _Get_current_module_handle( );
	const auto index = std::distance(storage.begin( ), std::ranges::find(storage, current_base, &basic_info::base));
	current_holder = index;
}

struct duplicate_info
{
	//offset from start to first object
	size_t from_start;
	//offset from object to duplicate
	size_t from_obj;
};

template<typename Proj>
static std::optional<duplicate_info> _Test_duplicate(const modules_storage_data& storage, Proj proj)
{
	const auto start = storage.begin( );
	const auto end = storage.end( );
	for (auto itr = start; itr != end; ++itr)
	{
		decltype(auto) obj1 = std::invoke(proj, *itr);
		for (auto itr_next = std::next(itr); itr_next != end; ++itr_next)
		{
			decltype(auto) obj2 = std::invoke(proj, *itr_next);
			if (obj1 == obj2)
				return duplicate_info(std::distance(start, itr), std::distance(itr, itr_next));
		}
	}
	return {};
}

bool modules_storage::update(bool force)
{
	modules_storage_data& storage = *this;

	if (!force && !storage.empty( ))
		return false;

	const auto basic_storage = _Get_all_modules( );

	if (storage.empty( ))
	{
		storage.assign(basic_storage.begin( ), basic_storage.end( ));
	}
	else if (std::ranges::equal(storage, basic_storage, [](const basic_info& l, const basic_info& r) {return l.base( ) == r.base( ); }))
	{
		return false;
	}
	else
	{
		modules_storage_data temp_storage;
		temp_storage.resize(basic_storage.size( ));
		std::vector<bool> stolen_data;
		stolen_data.resize(basic_storage.size( ), false);
		for (auto& rec : std::span(storage.begin( ), std::min(storage.size( ), basic_storage.size( ))))
		{
			const auto found = std::ranges::find(basic_storage, rec.base( ), &basic_info::base);
			if (found == basic_storage.end( ))
				continue;
			//record already exists, move it
			const auto idx = std::distance(basic_storage.begin( ), found);
			temp_storage[idx] = std::move(rec);
			stolen_data[idx] = true;
		}

		for (size_t idx = 0; idx < basic_storage.size( ); ++idx)
		{
			//stolen in previous loop
			if (stolen_data[idx])
				continue;

			//construct it from basic_info
			temp_storage[idx] = basic_storage[idx];
		}
		storage = std::move(temp_storage);
		runtime_assert(_Test_duplicate(storage, &info::name).has_value( ) == false);
	}

	_Write_current_idx(storage, current_index_);
	return true;
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
