module;

#include "info_includes.h"

export module nstd.rtlib:info;
export import :basic_info;
export import :exports;
export import :sections;
export import :vtables;
export import nstd.text.hashed_string;

export namespace nstd::rtlib
{
	struct info_string
	{
		using raw_type = nstd::text::hashed_wstring_view;

		//lowercase
		struct fixed_type : nstd::text::hashed_wstring
		{
			fixed_type( ) = default;

			fixed_type(const std::wstring_view str);
			fixed_type(const std::string_view str);

			template<typename C>
			fixed_type(const std::basic_string<C>& str) :fixed_type(std::basic_string_view<C>(str))
			{
			}

			fixed_type(nstd::text::hashed_wstring&& str) noexcept;
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
		info_string full_path;
		info_string name;
		info_string work_dir;

		info* root_class( ) override;
		const info* root_class( ) const override;

		info( ) = default;
		info(const basic_info& basic);
		info(info&& other) noexcept;
		info& operator=(info&& other) noexcept;

		template<typename T>
		bool operator==(const T& other)const
		{
			static_assert(std::derived_from<T, basic_info>, __FUNCSIG__": Incorrect type");

			const bool equal = *static_cast<const basic_info*>(this) == other;

			if constexpr (std::same_as<T, info>)
				return equal && full_path == other.full_path;
			else
				return equal;
		}
		template<typename T>
		bool operator!=(const T& other)const
		{
			return !(*this == other);
		}

		mem::block mem_block( ) const;
	};
}
