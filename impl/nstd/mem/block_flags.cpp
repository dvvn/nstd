module;

#include <windows.h>

#include <span>
#include <stdexcept>

module nstd.mem.block;

using namespace nstd::mem;

// ReSharper disable once CppInconsistentNaming
class MEMORY_BASIC_INFORMATION_UPDATER : protected MEMORY_BASIC_INFORMATION
{
	static constexpr SIZE_T class_size = sizeof(MEMORY_BASIC_INFORMATION);

public:
  DWORD flags() const
  {
      return reinterpret_cast<const DWORD&>(Protect);
  }

  SIZE_T size() const
  {
      return this->RegionSize;
  }

  DWORD state() const
  {
      return this->State;
	}

	bool valid;

	MEMORY_BASIC_INFORMATION_UPDATER(LPCVOID address)
	{
		valid = VirtualQuery(address, this, class_size) == class_size;
	}

	MEMORY_BASIC_INFORMATION_UPDATER(HANDLE process, LPCVOID address)
	{
		valid = VirtualQueryEx(process, address, this, class_size) == class_size;
	}
};

template <typename Fn>
static bool _Flags_iterator(std::span<uint8_t> mblock, Fn func = {})
{
    for (;;)
    {
        const MEMORY_BASIC_INFORMATION_UPDATER info = mblock.data();
        if (!info.valid)
            return false;
        // memory isn't commit!
        if (info.state() != MEM_COMMIT)
            return false;
        using ret_t = std::invoke_result_t<Fn, DWORD>;
        if constexpr (std::is_void_v<ret_t>)
        {
            std::invoke(func, info.flags());
        }
        else
        {
            static_assert(std::same_as<bool, ret_t>);
            // flags check isn't passed!
            if (!std::invoke(func, info.flags()))
                return false;
        }
        // found good result
        if (info.size() >= mblock.size())
            return true;
        // check next block
        mblock = mblock.subspan(info.size());
    }
}

static bool _Have_flags(const block* mblock, DWORD flags)
{
    return _Flags_iterator(*mblock, [flags](DWORD mem_flags) {
        return !!(mem_flags & flags);
    });
}

static bool _Dont_have_flags(const block* mblock, DWORD flags)
{
    return _Flags_iterator(*mblock, [flags](DWORD mem_flags) {
        return !(mem_flags & flags);
    });
}

bool block::have_flags(DWORD flags) const
{
    return _Have_flags(this, flags);
}

bool block::dont_have_flags(DWORD flags) const
{
    return _Dont_have_flags(this, flags);
}

DWORD block::get_flags() const
{
	DWORD flags = 0;
	const auto result = _Flags_iterator(*this, [&](DWORD current_flags)
	{
		flags |= current_flags;
	});
	return result ? flags : 0;
}

constexpr DWORD _Page_read_flags = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
constexpr DWORD _Page_write_flags = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOMBINE;
constexpr DWORD _Page_execute_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

bool block::readable() const
{
	return _Dont_have_flags(this, PAGE_NOACCESS);
}

bool block::readable_ex() const
{
	return _Have_flags(this, _Page_read_flags);
}

bool block::writable() const
{
	return _Have_flags(this, _Page_write_flags);
}

bool block::executable() const
{
	return _Have_flags(this, _Page_execute_flags);
}

bool block::code_padding() const
{
	//todo: move outside
	const auto first = this->front( );
	if(first != 0x00 && first != 0x90 && first != 0xCC)
		return false;
	for(const auto val : std::span(this->begin( ) + 1, this->end( )))
	{
		if(val != first)
			return false;
	}
	return true;
}
