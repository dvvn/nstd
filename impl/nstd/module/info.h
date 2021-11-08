#pragma once

//#include "module info/exports_storage.h"
//#include "module info/sections_storage.h"
//#include "module info/vtables_storage.h"

#include "exports.h"
#include "sections.h"
#include "vtables.h"

#include "nstd/one_instance.h"

namespace std
{
	template <class _Fty>
	class function;
}

// ReSharper disable CppInconsistentNaming
#ifndef _WINTERNL_
struct _LDR_DATA_TABLE_ENTRY;
using LDR_DATA_TABLE_ENTRY = _LDR_DATA_TABLE_ENTRY;
#endif
#ifndef _WINNT_
struct _IMAGE_DOS_HEADER;
using IMAGE_DOS_HEADER = _IMAGE_DOS_HEADER;
#ifdef _WIN64
struct _IMAGE_NT_HEADERS64;
using IMAGE_NT_HEADERS = _IMAGE_NT_HEADERS64;
#else
struct _IMAGE_NT_HEADERS;
using IMAGE_NT_HEADERS = _IMAGE_NT_HEADERS;
#endif
#endif
#ifndef _MINWINDEF_
using DWORD = unsigned long;
#endif
// ReSharper restore CppInconsistentNaming

namespace nstd::module
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
		~info( ) override;

		info(info&&) noexcept;
		info& operator=(info&&) noexcept;

		info(LDR_DATA_TABLE_ENTRY* ldr_entry, IMAGE_DOS_HEADER* dos, IMAGE_NT_HEADERS* nt);

		//module handle
		address base( ) const;
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

		using sections_t = sections;
		using exports_t = exports;
		using vtables_t = vtables;

		const sections_t& sections( ) const;
		const exports_t& exports( ) const;
		const vtables_t& vtables( ) const;

		sections_t& sections( );
		exports_t& exports( );
		vtables_t& vtables( );
	};

	class modules_storage
	{
	public:
		struct storage_type;

		modules_storage(const modules_storage&)            = delete;
		modules_storage& operator=(const modules_storage&) = delete;

		modules_storage(modules_storage&&) noexcept;
		modules_storage& operator=(modules_storage&&) noexcept;

		modules_storage( );
		~modules_storage( );

		modules_storage& update(bool force = false);

		info& current( ) const;
		info& owner( );

		using find_fn = std::function<bool(const info&)>;
		info* find(const find_fn& fn);
		info* rfind(const find_fn& fn);

	private:
		std::unique_ptr<storage_type> storage_;
		info* current_cached_ = nullptr;
	};

	namespace detail
	{
		struct all_modules_impl : modules_storage
		{
			all_modules_impl( )
			{
				this->update( );
			}
		};
	}

	using all_modules = one_instance<detail::all_modules_impl>;
}
