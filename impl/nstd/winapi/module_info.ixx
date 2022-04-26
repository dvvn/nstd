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

		module_info(pointer const entry);

		pointer operator->( ) const noexcept;
		reference operator*( ) const noexcept;

		std::wstring_view path( ) const noexcept;
		std::wstring_view name( ) const noexcept;

	private:
		pointer entry_;
	};
}