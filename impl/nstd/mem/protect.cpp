module;

#include "address_includes.h"
#include <optional>
#include <stdexcept>
#include <Windows.h>

module nstd.mem:protect;

using namespace nstd::mem;

protect::protect(protect&& other) noexcept
{
	*this = std::move(other);
}

protect& protect::operator=(protect&& other) noexcept
{
	using std::swap;
	swap(this->info_, other.info_);
	return *this;
}

protect::protect(const LPVOID addr, SIZE_T size, DWORD new_flags)
{
	DWORD old_flags;
	if (!VirtualProtect(addr, size, new_flags, std::addressof(old_flags)))
		throw std::runtime_error("Unable to protect the memory!");
	info_.emplace(addr, size, old_flags);
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
