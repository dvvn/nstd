module;

#include "block_includes.h"
#include <windows.h>
#include <stdexcept>

module nstd.mem.block;

using namespace nstd::mem;

// ReSharper disable once CppInconsistentNaming
class MEMORY_BASIC_INFORMATION_UPDATER : protected MEMORY_BASIC_INFORMATION
{
	static constexpr SIZE_T class_size = sizeof(MEMORY_BASIC_INFORMATION);

public:
	DWORD flags( ) const { return reinterpret_cast<const DWORD&>(Protect); }
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

static constexpr DWORD _Page_read_flags = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
static constexpr DWORD _Page_write_flags = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOMBINE;
static constexpr DWORD _Page_execute_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

template <typename Fn>
static bool _Flags_checker(block mblock, Fn checker = {})
{
	try
	{
		for (;;)
		{
			const MEMORY_BASIC_INFORMATION_UPDATER info = mblock.data( );

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

static bool _Have_flags(const block* mblock, DWORD flags)
{
	return _Flags_checker(*mblock, [flags](DWORD mem_flags)
						  {
							  return !!(mem_flags & flags);
						  });
}

static bool _Dont_have_flags(const block* mblock, DWORD flags)
{
	return _Flags_checker(*mblock, [flags](DWORD mem_flags)
						  {
							  return !(mem_flags & flags);
						  });
}

#ifdef NSTD_MEM_BLOCK_CHECK_CUSTOM_FLAGS
bool block::have_flags(DWORD flags) const
{
	return _Have_flags(this, flags);
}

bool block::dont_have_flags(DWORD flags) const
{
	return _Dont_have_flags(this, flags);
}
#endif

bool block::readable( ) const
{
	return _Dont_have_flags(this, PAGE_NOACCESS);
}

bool block::readable_ex( ) const
{
	return _Have_flags(this, _Page_read_flags);
}

bool block::writable( ) const
{
	return _Have_flags(this, _Page_write_flags);
}

bool block::executable( ) const
{
	return _Have_flags(this, _Page_execute_flags);
}

bool block::code_padding( ) const
{
	//todo: move outside
	const auto first = this->front( );
	if (first != 0x00 && first != 0x90 && first != 0xCC)
		return false;
	for (const auto val : this->subblock(1))
	{
		if (val != first)
			return false;
	}
	return true;
}
