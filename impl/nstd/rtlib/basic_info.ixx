module;

#include "basic_info_includes.h"

export module nstd.rtlib:basic_info;

export namespace nstd::rtlib
{
	class basic_info
	{
		LDR_DATA_TABLE_ENTRY* ldr_entry_ = nullptr;
		IMAGE_DOS_HEADER* dos_ = nullptr;
		IMAGE_NT_HEADERS* nt_ = nullptr;
	public:

		basic_info( ) = default;
		basic_info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt);

		basic_info(const basic_info& other) = default;
		basic_info& operator=(const basic_info& other) = default;

		basic_info(basic_info&& other)noexcept;
		basic_info& operator=(basic_info&& other)noexcept;

		//module handle
		void* base( ) const;

		LDR_DATA_TABLE_ENTRY* ENTRY( )const;
		IMAGE_DOS_HEADER* DOS( ) const;
		IMAGE_NT_HEADERS* NT( ) const;
	};
}