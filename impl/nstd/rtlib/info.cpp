module;

#include "nstd/ranges.h"
#include "nstd/runtime_assert.h"
#include "info_includes.h"

#include <functional>
#include <cwctype>

module nstd.rtlib:info;
import nstd.text.actions;
import nstd.container.wrapper;

using namespace nstd;
using namespace rtlib;
namespace rv = std::views;

info_string::fixed_type::fixed_type(const std::wstring_view str)
	:hashed_wstring(nstd::append<std::wstring>(str | rv::transform(text::to_lower)))
{
}

info_string::fixed_type::fixed_type(const std::string_view str)
	: hashed_wstring(nstd::append<std::wstring>(str | rv::transform(nstd::cast_all<wchar_t>) | rv::transform(text::to_lower)))
{
}

info_string::fixed_type::fixed_type(hashed_wstring&& str)noexcept
	: hashed_wstring(std::move(str))
{
	runtime_assert(fixed_type(str) == *this, "Incorrect passed string!");
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

info::info(const basic_info& basic)
	:basic_info(basic)
{
	if (!basic)
		return;

	const std::wstring_view path = {ENTRY( )->FullDllName.Buffer, ENTRY( )->FullDllName.Length / sizeof(wchar_t)};

	full_path = path;
	const auto name_bg = path.find_last_of('\\');
	name = path.substr(name_bg + 1);
	work_dir = path.substr(0, name_bg);
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

	swap(full_path, other.full_path);
	swap(name, other.name);
	swap(work_dir, other.work_dir);

	return *this;
}

block info::mem_block( ) const
{
	return {(uint8_t*)DOS( ), NT( )->OptionalHeader.SizeOfImage};
}

//DWORD info::check_sum( ) const
//{
//	return NT( )->OptionalHeader.CheckSum;
//}
//
//DWORD info::code_size( ) const
//{
//	return NT( )->OptionalHeader.SizeOfCode;
//}
//
//DWORD info::image_size( ) const
//{
//	return NT( )->OptionalHeader.SizeOfImage;
//}