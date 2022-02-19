module;

#include "nstd/ranges.h"
#include "nstd/runtime_assert.h"
#include "info_includes.h"

#include <functional>
#include <cwctype>

module nstd.rtlib:info;

using namespace nstd;
using namespace mem;
using namespace rtlib;

template<typename C, typename Fn>
static auto _To_lower(const std::basic_string_view<C> str, Fn fn)
{
	std::wstring out;
	const auto lstr = std::views::transform(str, fn);
	out.assign(lstr.begin( ), lstr.end( ));
	return out;
}

info_string::fixed_type::fixed_type(const std::wstring_view str)
	:hashed_wstring(_To_lower(str, [](wchar_t c) {return std::towlower(c); }))
{

}

info_string::fixed_type::fixed_type(const std::string_view str)
	: hashed_wstring(_To_lower(str, [](char c) {return static_cast<char>(std::tolower(c)); }))
{

}

info_string::info_string(const std::wstring_view raw_string)
	: raw(raw_string), fixed(raw_string)
{
	fixed = raw_string;
	//if raw_string already lowercase, dont recalc the hash
	if (fixed == raw_string)
		raw = {raw_string, fixed.hash( )};
	else
		raw = {raw_string};

}

info* info::root_class( ) { return this; }
const info* info::root_class( ) const { return this; }

info::info(basic_info&& basic) noexcept
	:basic_info(std::move(basic))
{
	const std::wstring_view path = {ENTRY( )->FullDllName.Buffer, ENTRY( )->FullDllName.Length / sizeof(wchar_t)};

	full_path_ = path;
	const auto name_bg = path.find_last_of('\\');
	name_ = path.substr(name_bg + 1);
	work_dir_ = path.substr(0, name_bg);
}

static basic_info _Copy_info(const basic_info& basic)
{
	return basic;
}

info::info(const basic_info& basic) :info(_Copy_info(basic))
{
}

info::info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt)
	: info(basic_info(ldr_entry, dos, nt))
{

}

info::info(info&& other) noexcept
{
	*this = std::move(other);
}

info& info::operator=(info&& other) noexcept
{
	using std::swap;
	using nstd::swap;
	swap<basic_info>(*this, other);
	swap(this->sections( ), other.sections( ));
	swap(this->exports( ), other.exports( ));
	swap(this->vtables( ), other.vtables( ));

	swap(full_path_, other.full_path_);
	swap(name_, other.name_);
	swap(work_dir_, other.work_dir_);

	return *this;
}

block info::mem_block( ) const
{
	return {base( ).pointer, image_size( )};
}

DWORD info::check_sum( ) const
{
	return NT( )->OptionalHeader.CheckSum;
}

DWORD info::code_size( ) const
{
	return NT( )->OptionalHeader.SizeOfCode;
}

DWORD info::image_size( ) const
{
	return NT( )->OptionalHeader.SizeOfImage;
}

const info_string& info::full_path( ) const
{
	//return {this->ldr_entry_->FullDllName.Buffer, this->ldr_entry_->FullDllName.Length / sizeof(wchar_t)};
	return full_path_;
}

const info_string& info::name( ) const
{
	return name_;
}

const info_string& info::work_dir( ) const
{
	/*auto path_to = full_path( );
	path_to.remove_suffix(raw_name( ).size( ));
	return path_to;*/
	return work_dir_;
}