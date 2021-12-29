module;

#include "nstd/ranges.h"
#include "nstd/runtime_assert.h"
#include "info_includes.h"

#include <functional>

module nstd.rtlib.info;

using namespace nstd;
using namespace nstd::rtlib;

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
