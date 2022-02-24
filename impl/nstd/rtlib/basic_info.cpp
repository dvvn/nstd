module;

#include "basic_info_includes.h"

module nstd.rtlib:basic_info;
import nstd.mem.address;

using namespace nstd::rtlib;

basic_info::basic_info(LDR_DATA_TABLE_ENTRY* ldr_entry)
{
#if 0
	runtime_assert(ldr_entry != nullptr);
	runtime_assert(dos != nullptr);
	runtime_assert(nt != nullptr);

	//manual_handle_ = winapi::module_handle(handle);

	ldr_entry_ = ldr_entry;
	dos_ = dos;
	nt_ = nt;
#endif
	ldr_entry_ = ldr_entry;
}

basic_info::operator bool( )const
{
	return ldr_entry_ != nullptr;
}

LDR_DATA_TABLE_ENTRY* basic_info::ENTRY( ) const
{
	return ldr_entry_;
}

IMAGE_DOS_HEADER* basic_info::DOS( ) const
{
	return static_cast<IMAGE_DOS_HEADER*>(ldr_entry_->DllBase);
}

IMAGE_NT_HEADERS* basic_info::NT( ) const
{
	const basic_address dos = this->DOS( );
	return dos + dos->e_lfanew;
}