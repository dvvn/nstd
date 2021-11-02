#include "block.h"

#include <stdexcept>
#include <windows.h>

using namespace nstd::mem;

using flags_type = block::flags_type;

// ReSharper disable once CppInconsistentNaming
class MEMORY_BASIC_INFORMATION_UPDATER : protected MEMORY_BASIC_INFORMATION
{
	static constexpr SIZE_T class_size = sizeof(MEMORY_BASIC_INFORMATION);

public:
	flags_type flags( ) const { return reinterpret_cast<const flags_type&>(Protect); }
	SIZE_T size( ) const { return this->RegionSize; }
	DWORD state( ) const { return this->State; }

	MEMORY_BASIC_INFORMATION_UPDATER(LPCVOID address)
	{
		if (VirtualQuery(address, this, class_size) != class_size)
			throw std::runtime_error("Unable to update memory information");
	}

	MEMORY_BASIC_INFORMATION_UPDATER(HANDLE process, LPCVOID address)
	{
		if (VirtualQueryEx(process, address, this, class_size) != class_size)
			throw std::runtime_error("Unable to update external memory information");
	}
};

static constexpr flags_type _Page_read_flags    = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
static constexpr flags_type _Page_write_flags   = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOMBINE;
static constexpr flags_type _Page_execute_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

template <typename Fn>
static bool _Flags_checker(block mblock, Fn checker = {})
{
	try
	{
		for (;;)
		{
			const auto info = MEMORY_BASIC_INFORMATION_UPDATER(mblock._Unchecked_begin( ));

			//memory isn't commit!
			if (info.state( ) != MEM_COMMIT)
				return false;
			//flags check isn't passed!
			if (!std::invoke(checker, info.flags( )))
				return false;
			//found good result
			if (info.size( ) >= mblock.size( ))
				return true;
			//check next block
			mblock = mblock.subblock(info.size( ));
		}
	}
	catch (const std::exception&)
	{
		return false;
	}
}

bool block::have_flags(flags_type flags) const
{
	return _Flags_checker(*this, [flags](flags_type mem_flags)
	{
		return (mem_flags & flags);
	});
}

bool block::dont_have_flags(flags_type flags) const
{
	return _Flags_checker(*this, [flags](flags_type mem_flags)
	{
		return !(mem_flags & flags);
	});
}

bool block::readable( ) const
{
	return this->dont_have_flags(PAGE_NOACCESS);
}

bool block::readable_ex( ) const
{
	return this->have_flags(_Page_read_flags);
}

bool block::writable( ) const
{
	return this->have_flags(_Page_write_flags);
}

bool block::executable( ) const
{
	return this->have_flags(_Page_execute_flags);
}

bool block::code_padding( ) const
{
	const auto first = this->front( );
	if (first != 0x00 && first != 0x90 && first != 0xCC)
		return false;
	for (const auto val: this->subblock(1))
	{
		if (val != first)
			return false;
	}
	return true;
}
