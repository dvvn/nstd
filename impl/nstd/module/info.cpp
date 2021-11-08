#include "info.h"

#include <nstd/runtime_assert_fwd.h>

#include <Windows.h>
#include <winternl.h>

#include <functional>
#include <filesystem>
#include <ranges>

using namespace nstd;
using namespace nstd::module;

info* info::root_class( ) { return this; }
const info* info::root_class( ) const { return this; }

info::~info( )                         = default;
info::info(info&&) noexcept            = default;
info& info::operator=(info&&) noexcept = default;

info::info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt)
{
	runtime_assert(ldr_entry != nullptr);
	runtime_assert(dos != nullptr);
	runtime_assert(nt != nullptr);

	//manual_handle_ = winapi::module_handle(handle);

	this->ldr_entry = ldr_entry;
	this->dos       = dos;
	this->nt        = nt;

	const auto raw_name = this->raw_name( );
	const auto wname    = std::views::transform(raw_name, towlower);
	this->name_.append(wname.begin( ), wname.end( ));
	this->name_is_unicode_ = IsTextUnicode(raw_name._Unchecked_begin( ), raw_name.size( ) * sizeof(wchar_t), nullptr);
}

address info::base( ) const
{
	return this->ldr_entry->DllBase;
}

mem::block info::mem_block( ) const
{
	return {base( ), image_size( )};
}

// ReSharper disable CppInconsistentNaming

IMAGE_DOS_HEADER* info::DOS( ) const
{
	return this->dos;
}

IMAGE_NT_HEADERS* info::NT( ) const
{
	return this->nt;
}

DWORD info::check_sum( ) const
{
	return NT( )->OptionalHeader.CheckSum;
}

// ReSharper restore CppInconsistentNaming

DWORD info::code_size( ) const
{
	return NT( )->OptionalHeader.SizeOfCode;
}

DWORD info::image_size( ) const
{
	return NT( )->OptionalHeader.SizeOfImage;
}

std::wstring_view info::work_dir( ) const
{
	auto path_to = full_path( );
	path_to.remove_suffix(raw_name( ).size( ));
	return path_to;
}

std::wstring_view info::full_path( ) const
{
	return {this->ldr_entry->FullDllName.Buffer, this->ldr_entry->FullDllName.Length / sizeof(wchar_t)};
}

std::wstring_view info::raw_name( ) const
{
	const auto path = full_path( );
	return path.substr(path.find_last_of('\\') + 1);
}

const std::wstring& info::name( ) const
{
	return this->name_;
}

bool info::name_is_unicode( ) const
{
	return this->name_is_unicode_;
}

auto info::sections( ) const -> const sections_t& { return *this; }
auto info::exports( ) const -> const exports_t& { return *this; }
auto info::vtables( ) const -> const vtables_t& { return *this; }

auto info::sections( ) -> sections_t& { return *this; }
auto info::exports( ) -> exports_t& { return *this; }
auto info::vtables( ) -> vtables_t& { return *this; }

//-------------------

struct modules_storage::storage_type : std::list<info>
{
	storage_type(const storage_type&)            = delete;
	storage_type& operator=(const storage_type&) = delete;

	storage_type(storage_type&&)            = default;
	storage_type& operator=(storage_type&&) = default;

	storage_type( ) = default;
};

modules_storage::modules_storage(modules_storage&&) noexcept = default;

modules_storage& modules_storage::operator=(modules_storage&&) noexcept = default;

modules_storage::modules_storage( )
{
	storage_ = std::make_unique<storage_type>( );
}

modules_storage::~modules_storage( ) = default;

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
	result.nt  = nt;

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

	runtime_assert(std::ranges::adjacent_find(container, { }, &info::name) == container.end( ));
	return container;
}

//module_info_loaded::module_info_loaded(const boost::filesystem::path& p, DWORD flags):
//    handle_(LoadLibraryExW(p.c_str( ), nullptr, flags))
//{
//    runtime_assert(handle_!=nullptr, "Unable to load module");
//
//
//    auto all_modules=get_all_modules()
//
//}

static volatile DECLSPEC_NOINLINE HMODULE _Get_current_module_handle( )
{
	MEMORY_BASIC_INFORMATION info;
	constexpr SIZE_T info_size = sizeof(MEMORY_BASIC_INFORMATION);

	//todo: is this is dll, try to load this fuction from inside
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

	if (storage_->empty( ))
	{
		runtime_assert(current_cached_ == nullptr, "Cache already set");

		for (auto& m: _Get_all_modules( ))
		{
			auto& item = storage_->emplace_back(std::move(m));
			if (current_cached_ == nullptr && item.base( ) == current_base)
				current_cached_ = std::addressof(item);
		}
	}
	else if (force)
	{
		auto all = _Get_all_modules( );

		//erase all unused modules
		storage_->remove_if([&](const info& m)-> bool
		{
			for (const auto& m_new: all)
			{
				if (m.base( ) == m_new.base( ))
					return false;
			}
			return true;
		});

		auto find_current = current_cached_ != nullptr;

		//add all new modules, save the order
		auto itr = storage_->begin( );
		for (auto& m: all)
		{
			if (itr == storage_->end( ) || itr->base( ) != m.base( ))
				itr = storage_->insert(itr, const_cast<info&&>(m));

			if (find_current && itr->base( ) == current_base)
			{
				current_cached_ = std::addressof(*itr);
				find_current    = false;
			}

			++itr;
		}
	}

	return *this;
}

info& modules_storage::owner( )
{
	return storage_->front( );
}

template <typename T, typename Pred>
static info* _Find(T&& storage, Pred&& pred)
{
	for (info& i: storage)
	{
		if (std::invoke(pred, i))
			return std::addressof(i);
	}


	return nullptr;
}

info* modules_storage::find(const find_fn& fn)
{
	return _Find(*storage_, fn);
}

info* modules_storage::rfind(const find_fn& fn)
{
	return _Find(*storage_ | std::views::reverse, fn);
}