#pragma once
#include <optional>

//using UINT_PTR =
//#ifdef _W64
//_W64
//#endif
//unsigned
//#if defined(_WIN64)
//__int64
//#else
//int;
//#endif

using ULONG_PTR =
#ifdef _W64
_W64
#endif
unsigned
#if defined(_WIN64)
__int64
#else
long;
#endif

using SIZE_T = ULONG_PTR;
using DWORD = unsigned long;
using LPVOID = void*;

namespace nstd
{
	class memory_block;
	class address;

	class memory_protect
	{
	public:
		struct value_type
		{
			LPVOID addr;
			SIZE_T size;
			DWORD  flags;
		};

		memory_protect(const memory_protect&)            = delete;
		memory_protect& operator=(const memory_protect&) = delete;

		memory_protect(memory_protect&& other) noexcept;
		memory_protect& operator=(memory_protect&& other) noexcept;

		memory_protect( ) = default;

		memory_protect(LPVOID addr, SIZE_T size, DWORD new_flags);
		memory_protect(address addr, SIZE_T size, DWORD new_flags);
		memory_protect(const memory_block& mem, DWORD new_flags);

		~memory_protect( );

		bool restore( );
		bool has_value( ) const;

	private:
		std::optional<value_type> info_;
	};
}
