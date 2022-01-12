module;

#include "info_includes.h"

export module nstd.rtlib:info;
export import :basic_info;
export import :exports;
export import :sections;
export import :vtables;

export namespace nstd::rtlib
{
	struct info_string
	{
		hashed_wstring_view raw;
		hashed_wstring fixed;//lowercase

		info_string( ) = default;
		info_string(const std::wstring_view& raw_string);
	};

	struct info final : basic_info, sections_storage, exports_storage, vtables_storage
	{
	private:
		info_string full_path_;
		info_string name_;
		info_string work_dir_;

		info* root_class( ) override;
		const info* root_class( ) const override;
	public:

		info(const basic_info& basic);
		info(basic_info&& basic)noexcept;
		info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt);
		info(info&& other) noexcept;
		info& operator=(info&& other) noexcept;

		mem::block mem_block( ) const;

		DWORD check_sum( ) const;
		DWORD code_size( ) const;
		DWORD image_size( ) const;

		const info_string& full_path( ) const;
		const info_string& name( ) const;
		const info_string& work_dir( ) const;
	};
}
