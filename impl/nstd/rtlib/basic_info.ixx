module;

#include "basic_info_includes.h"

export module nstd.rtlib:basic_info;

export namespace nstd::rtlib
{
	class basic_info
	{
		LDR_DATA_TABLE_ENTRY* ldr_entry_ = nullptr;
		/*IMAGE_DOS_HEADER* dos_ = nullptr;
		IMAGE_NT_HEADERS* nt_ = nullptr;*/
	public:

		basic_info(LDR_DATA_TABLE_ENTRY* ldr_entry = nullptr);

		LDR_DATA_TABLE_ENTRY* ENTRY( )const;
		//DllBase (module handle)
		IMAGE_DOS_HEADER* DOS( ) const;
		IMAGE_NT_HEADERS* NT( ) const;

		template<std::same_as<basic_info> T>
		bool operator==(T other)const
		{
			return ldr_entry_ == other.ldr_entry_;
		}

		template<class T>
		bool operator!=(T other)const
		{
			return !(*this == other);
		}

		explicit operator bool( )const;
	};

}