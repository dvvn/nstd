module;

#include "basic_info_includes.h"

module nstd.rtlib:basic_info;

using namespace nstd;
using namespace rtlib;

basic_info::basic_info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt)
{
	runtime_assert(ldr_entry != nullptr);
	runtime_assert(dos != nullptr);
	runtime_assert(nt != nullptr);

	//manual_handle_ = winapi::module_handle(handle);

	ldr_entry_ = ldr_entry;
	dos_ = dos;
	nt_ = nt;
}

mem::address basic_info::base( ) const
{
	return ldr_entry_->DllBase;
}

LDR_DATA_TABLE_ENTRY* basic_info::ENTRY( ) const
{
	return ldr_entry_;
}

IMAGE_DOS_HEADER* basic_info::DOS( ) const
{
	return dos_;
}

IMAGE_NT_HEADERS* basic_info::NT( ) const
{
	return nt_;
}