#pragma once

// ReSharper disable CppInconsistentNaming
#ifdef _WIN64
struct _IMAGE_NT_HEADERS64;
using IMAGE_NT_HEADERS = _IMAGE_NT_HEADERS64;
#else
struct _IMAGE_NT_HEADERS;
using IMAGE_NT_HEADERS = _IMAGE_NT_HEADERS;
#endif
struct _LDR_DATA_TABLE_ENTRY;
using LDR_DATA_TABLE_ENTRY = _LDR_DATA_TABLE_ENTRY;
struct _IMAGE_DOS_HEADER;
using IMAGE_DOS_HEADER = _IMAGE_DOS_HEADER;
struct _IMAGE_SECTION_HEADER;
using IMAGE_SECTION_HEADER = _IMAGE_SECTION_HEADER;
using DWORD = unsigned long;
// ReSharper restore CppInconsistentNaming

namespace nstd::os
{
	class module_info;
}

namespace nstd::os::detail
{
	struct root_class_getter
	{
	protected:
		virtual ~root_class_getter( ) = default;

		virtual module_info*       root_class( ) =0;
		virtual const module_info* root_class( ) const =0;

	public:
		virtual void lock( ) const =0;
		virtual void unlock( ) const =0;
	};
}
