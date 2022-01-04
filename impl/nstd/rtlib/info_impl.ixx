module;

#include "info_includes.h"

export module nstd.rtlib:info;
export import :exports;
export import :sections;
export import :vtables;

export namespace nstd::rtlib
{
	class info final : sections, exports, vtables
	{
		LDR_DATA_TABLE_ENTRY* ldr_entry;
		IMAGE_DOS_HEADER* dos;
		IMAGE_NT_HEADERS* nt;

		std::wstring name_;
		bool name_is_unicode_;

	protected:
		info* root_class( ) override;
		const info* root_class( ) const override;

	public:
		info(info&& other) noexcept;
		info& operator=(info&& other) noexcept;

		info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt);

		//module handle
		mem::address base( ) const;
		mem::block mem_block( ) const;

		// ReSharper disable CppInconsistentNaming

		IMAGE_DOS_HEADER* DOS( ) const;
		IMAGE_NT_HEADERS* NT( ) const;

		// ReSharper restore CppInconsistentNaming

		DWORD check_sum( ) const;
		DWORD code_size( ) const;
		DWORD image_size( ) const;

		std::wstring_view work_dir( ) const;
		std::wstring_view full_path( ) const;
		std::wstring_view raw_name( ) const;

		const std::wstring& name( ) const;
		bool name_is_unicode( ) const;

		using sections_t = rtlib::sections;
		using exports_t = rtlib::exports;
		using vtables_t = rtlib::vtables;

		const sections_t& sections( ) const;
		const exports_t& exports( ) const;
		const vtables_t& vtables( ) const;

		sections_t& sections( );
		exports_t& exports( );
		vtables_t& vtables( );
	};

}
