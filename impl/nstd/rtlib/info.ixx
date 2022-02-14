module;


#include "info_includes.h"


export module nstd.rtlib:info;
export import :basic_info;
export import :exports;
export import :sections;
export import :vtables;
export import nstd;

export namespace nstd::rtlib
{
	struct info_string
	{
		/*struct raw_type :hashed_wstring_view
		{
			template<typename ...T>
			raw_type(T&& ...args) :hashed_wstring_view(std::forward<T>(args)...) { }
		};*/

		using raw_type = hashed_wstring_view;

		//lowercase
		struct fixed_type :hashed_wstring
		{
			fixed_type( ) = default;

			fixed_type(const std::wstring_view str);
			fixed_type(const std::string_view str);

			template<typename C>
			fixed_type(const std::basic_string<C>& str) :fixed_type(std::basic_string_view<C>(str))
			{
			}

		};

		raw_type raw;
		fixed_type fixed;

		info_string( ) = default;
		info_string(const std::wstring_view raw_string);

		template<class T>
		bool operator==(const T& other)const
		{
			if constexpr (std::same_as<T, info_string>)
				return fixed == other.fixed;
			else if constexpr (std::same_as<T, fixed_type>)
				return fixed == other;
			else if constexpr (std::same_as<T, raw_type>)
				return raw == other;
			else
			{
				static_assert(false, "Incorrect type");
				return false;
			}
		}
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

		info( ) = default;
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
