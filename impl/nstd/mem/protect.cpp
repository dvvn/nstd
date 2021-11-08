#include "protect.h"
#include "block.h"

#include "nstd/address.h"

#include <Windows.h>

#include <stdexcept>
using namespace nstd::mem;

protect::protect(protect&& other) noexcept
{
	*this = std::move(other);
}

protect& protect::operator=(protect&& other) noexcept
{
	std::swap(this->info_, other.info_);
	return *this;
}

protect::protect(const LPVOID addr, SIZE_T size, DWORD new_flags)
{
	DWORD old_flags;
	if (!VirtualProtect(addr, size, new_flags, std::addressof(old_flags)))
		throw std::runtime_error("Unable to protect the memory!");
	info_.emplace(addr, size, old_flags);
}

protect::protect(address addr, SIZE_T size, DWORD new_flags)
	: protect(addr.ptr<void>( ), size, new_flags)
{
}

protect::protect(const block& mem, DWORD new_flags)
	: protect((mem._Unchecked_begin( )), mem.size( ), new_flags)
{
}

template <bool FromDestructor>
static auto _Restore_impl(std::optional<protect::value_type>& info)
{
	if (info.has_value( ))
	{
		auto& [addr, size, flags] = *info;
		if (DWORD unused; VirtualProtect(addr, size, flags, std::addressof(unused)))
		{
			if constexpr (!FromDestructor)
			{
				info.reset( );
				return true;
			}
		}
	}
	if constexpr (!FromDestructor)
		return false;
}

protect::~protect( )
{
	_Restore_impl<true>(info_);
}

bool protect::restore( )
{
	return _Restore_impl<false>(info_);
}

bool protect::has_value( ) const
{
	return info_.has_value( );
}