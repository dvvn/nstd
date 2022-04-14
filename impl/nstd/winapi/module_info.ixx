module;

#include <windows.h>
#include <winternl.h>

#include <string_view>

export module nstd.winapi.module_info;

export namespace nstd::winapi
{
	class module_info
	{
	public:
		using pointer = LDR_DATA_TABLE_ENTRY*;
		using reference = LDR_DATA_TABLE_ENTRY&;

		using _Str = std::basic_string<WCHAR>;
		using _Strv = std::basic_string_view<WCHAR>;

		module_info(pointer const entry);

		pointer operator->( ) const noexcept;
		reference operator*( ) const noexcept;

		_Strv path( ) const noexcept;
		_Strv name( ) const noexcept;

	private:
		pointer entry_;
	};
}