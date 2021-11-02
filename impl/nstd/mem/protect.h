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
	class address;
}

namespace nstd::mem
{
	class block;

	class protect
	{
	public:
		struct value_type
		{
			LPVOID addr;
			SIZE_T size;
			DWORD flags;
		};

		protect(const protect&)            = delete;
		protect& operator=(const protect&) = delete;

		protect(protect&& other) noexcept;
		protect& operator=(protect&& other) noexcept;

		protect( ) = default;

		protect(LPVOID addr, SIZE_T size, DWORD new_flags);
		protect(address addr, SIZE_T size, DWORD new_flags);
		protect(const block& mem, DWORD new_flags);

		~protect( );

		bool restore( );
		bool has_value( ) const;

	private:
		std::optional<value_type> info_;
	};
}
