module;

#include "includes.h"

module nstd.rtlib:all_infos;
import :info;
import nstd.mem.address;
import nstd.lazy_invoke;

using namespace nstd;
using namespace rtlib;

static DECLSPEC_NOINLINE HMODULE _Get_current_module_handle( )
{
	MEMORY_BASIC_INFORMATION info;
	constexpr SIZE_T info_size = sizeof(MEMORY_BASIC_INFORMATION);

	//todo: is this is dll, try to load this function from inside
	const auto len = VirtualQueryEx(GetCurrentProcess( ), _Get_current_module_handle, std::addressof(info), info_size);
	runtime_assert(len == info_size, "Wrong size");
	return static_cast<HMODULE>(info.AllocationBase);
}

static size_t _Get_current_idx(const modules_storage_data& storage)
{
	const auto current_base = _Get_current_module_handle( );
	for (size_t i = 0; i < storage.size( ); i++)
	{
		if (reinterpret_cast<HMODULE>(storage[i].DOS( )) == current_base)
			return i;
	}
	runtime_assert("Unable to find current module!");
	return static_cast<size_t>(-1);
}

modules_storage& modules_storage::operator=(modules_storage_data&& other) noexcept
{
	current_index_ = _Get_current_idx(other);

	using std::swap;
	swap<modules_storage_data>(*this, other);
	return *this;
}

struct header
{
	PIMAGE_DOS_HEADER dos;
	PIMAGE_NT_HEADERS nt;
};

static std::optional<header> _Get_file_headers(address base)
{
	IMAGE_DOS_HEADER* const dos = base;

	// check for invalid DOS / DOS signature.
	if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE /* 'MZ' */)
		return {};

	// get NT headers.
	IMAGE_NT_HEADERS* const nt = basic_address(dos) + dos->e_lfanew;

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

template<typename T>
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
		container.emplace_back(ldr_entry/*, headers->dos, headers->nt*/);
	}

	return container;
}

struct duplicate_info
{
	//offset from start to first object
	size_t from_start;
	//offset from object to duplicate
	size_t from_obj;
};

static std::optional<duplicate_info> _Test_duplicate(const modules_storage_data& storage)
{
	const auto start = storage.begin( );
	const auto end = storage.end( );
	for (auto itr = start; itr != end; ++itr)
	{
		const auto& name1 = itr->name;
		for (auto itr_next = std::next(itr); itr_next != end; ++itr_next)
		{
			const auto& name2 = itr_next->name;
			if (name1 == name2)
				return duplicate_info(std::distance(start, itr), std::distance(itr, itr_next));
		}
	}
	return {};
}

static auto _Update_full(modules_storage_data& current_storage)
{
	modules_storage_data new_storage;
	const auto basic_storage = _Get_all_modules<basic_info>( );
	if (!std::ranges::equal(current_storage, basic_storage, std::equal_to<basic_info>( )))
	{
		new_storage.resize(basic_storage.size( ));
		std::vector<bool> stoled_info;
		stoled_info.resize(basic_storage.size( ), false);

		//steal already filled data
		for (auto& current_rec : std::span(current_storage.begin( ), std::min(current_storage.size( ), basic_storage.size( ))))
		{
			const auto found = std::ranges::find(basic_storage, static_cast<basic_info>(current_rec));
			//record not found, create it later
			if (found == basic_storage.end( ))
				continue;

			//record already exists, move it
			const auto idx = std::distance(basic_storage.begin( ), found);
			new_storage[idx] = std::move(current_rec);
			stoled_info[idx] = true;
		}

		for (size_t idx = 0; idx < basic_storage.size( ); ++idx)
		{
			//stolen in previous loop
			if (stoled_info[idx])
				continue;

			//construct it from basic_info
			new_storage[idx] = basic_storage[idx];
		}

		runtime_assert(!_Test_duplicate(new_storage));
	}

	return new_storage;
}

bool modules_storage::update(bool force)
{
	if (locked( ))
		throw std::logic_error("Unable to update locked storage!");

	modules_storage_data new_storage;

	if (this->empty( ))
		new_storage = _Get_all_modules<info>( );
	else if (force)
		new_storage = _Update_full(*this);

	if (new_storage.empty( ))
		return false;

	*this = std::move(new_storage);
	try_lock( );

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

bool modules_storage::locked( )const
{
	return locked_;
}

void modules_storage::unlock( )
{
	locked_ = false;
}

void modules_storage::set_locker(lock_fn&& fn)
{
	runtime_assert(fn != nullptr, "Unable to write empty function!");
	runtime_assert(!locked( ), "Storage is locked");
	locker_ = std::move(fn);
	try_lock( );
}

bool modules_storage::contains_locker( )const
{
	return locker_ != nullptr;
}

void modules_storage::try_lock( )
{
	runtime_assert(!locked( ) && contains_locker( ));
	locked_ = locker_(*this);
}