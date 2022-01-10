module;

#include "basic_info_includes.h"

export module nstd.rtlib:basic_info;
export import nstd.mem;

export namespace nstd::rtlib
{
	class basic_info
	{
		LDR_DATA_TABLE_ENTRY* ldr_entry_;
		IMAGE_DOS_HEADER* dos_;
		IMAGE_NT_HEADERS* nt_;
	public:

		basic_info( ) = default;
		basic_info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt);

		//module handle
		mem::address base( ) const;

		LDR_DATA_TABLE_ENTRY* ENTRY( )const;
		IMAGE_DOS_HEADER* DOS( ) const;
		IMAGE_NT_HEADERS* NT( ) const;
	};
}