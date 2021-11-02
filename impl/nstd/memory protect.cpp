#include "memory protect.h"

#include "address.h"
#include "memory block.h"

#include <Windows.h>

#include <stdexcept>
using namespace nstd;

memory_protect::memory_protect(memory_protect&& other) noexcept
{
	*this = std::move(other);
}

memory_protect& memory_protect::operator=(memory_protect&& other) noexcept
{
	std::swap(this->info_, other.info_);
	return *this;
}

memory_protect::memory_protect(const LPVOID addr, SIZE_T size, DWORD new_flags)
{
	DWORD old_flags;
	if (!VirtualProtect(addr, size, new_flags, std::addressof(old_flags)))
		throw std::runtime_error("Unable to protect the memory!");
	info_.emplace(addr, size, old_flags);
}

memory_protect::memory_protect(address addr, SIZE_T size, DWORD new_flags)
	: memory_protect(addr.ptr<void>( ), size, new_flags)
{
}

memory_protect::memory_protect(const memory_block& mem, DWORD new_flags)
	: memory_protect(mem.addr( ), mem.size( ), new_flags)
{
}

template <bool FromDestructor>
static auto _Restore_impl(std::optional<memory_protect::value_type>& info)
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

memory_protect::~memory_protect( )
{
	_Restore_impl<true>(info_);
}

bool memory_protect::restore( )
{
	return _Restore_impl<false>(info_);
}

bool memory_protect::has_value( ) const
{
	return info_.has_value( );
}
