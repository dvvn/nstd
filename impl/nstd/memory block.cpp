#include "memory block.h"

#include "runtime_assert_fwd.h"
#include "overload.h"
#include "signature.h"

#include <algorithm>
#include <optional>

#include <windows.h>

static_assert(std::same_as<unsigned long, DWORD>);

using namespace nstd;

memory_block::memory_block(const address& begin, size_type mem_size)
	: storage_type(begin.ptr<uint8_t>( ), mem_size)
{
}

memory_block::memory_block(const address& begin, const address& end)
	: storage_type(begin.ptr<uint8_t>( ), end.ptr<uint8_t>( ))
{
}

memory_block::memory_block(const address& addr)
	: memory_block(addr, sizeof(address))
{
}

memory_block::memory_block(const storage_type& span)
	: storage_type(span)
{
}

memory_block memory_block::find_block(std::span<const uint8_t> rng) const
{
	const auto rng_size = rng.size( );
	const auto limit    = this->size( ) - rng_size;

	const auto start1 = this->_Unchecked_begin( );
	const auto start2 = rng._Unchecked_begin( );

	for (auto offset = 0; offset < limit; ++offset)
	{
		const auto start = start1 + offset;
		if (std::memcmp(start, start2, rng_size) == 0)
			return {start, rng_size};
	}

	return {};
}

struct unknown_find_result
{
	struct
	{
		size_t before;
		size_t after;
	} scanned;

	bool found;
};

static unknown_find_result _Find_unknown_bytes(const memory_block& block, const signature_unknown_bytes& rng)
{
	size_t offset  = 0;
	size_t scanned = 0;

	const auto& storage = rng.storage( );
	if (!storage.empty( ))
	{
		const auto found = block.find_block(storage);
		if (found.empty( ))
			return {block.size( ), 0, false};
		scanned = std::distance(block._Unchecked_begin( ), found._Unchecked_begin( ));
		offset  = found.size( );
	}

	offset += rng.skip;
	auto rng2 = rng.next.get( );

	while (rng2 != nullptr)
	{
		const auto block2    = block.subblock(scanned + offset);
		const auto& storage2 = rng2->storage( );

		const auto start1 = block2._Unchecked_begin( );
		const auto start2 = storage2._Unchecked_begin( );
		if (std::memcmp(start1, start2, storage2.size( )) != 0)
			return {scanned, offset, false};

		offset += storage2.size( ) + rng2->skip;
		rng2 = rng2->next.get( );
	}

	return {scanned, offset, true};
}

memory_block memory_block::find_block(const signature_unknown_bytes& rng) const
{
	const auto rng_size = [&]
	{
		auto rng0   = std::addressof(rng);
		size_t size = 0;
		for (;;)
		{
			size += rng0->storage( ).size( );
			size += rng0->skip;

			if (!rng0->next)
				break;

			rng0 = rng0->next.operator->( );
		}

		return size;
	}( );

	auto block = *this;

	for (;;)
	{
		if (block.size( ) < rng_size)
			return {};

		const auto [scanned, found] = _Find_unknown_bytes(block, rng);
		if (found)
		{
			const auto result_start = std::next(block._Unchecked_begin( ), scanned.before);
			return {result_start, rng_size};
		}

		block = block.subblock(scanned.before + 1);
	}
}

address memory_block::addr( ) const
{
	return this->_Unchecked_begin( );
}

address memory_block::last_addr( ) const
{
	return this->_Unchecked_end( );
}

memory_block memory_block::subblock(size_t offset) const
{
	return memory_block(this->subspan(offset));
}

memory_block memory_block::shift_to(pointer ptr) const
{
	const auto offset = std::distance(this->_Unchecked_begin( ), ptr);
	return this->subblock(offset);
}

memory_block memory_block::shift_to_start(const memory_block& block) const
{
	return this->shift_to(block._Unchecked_begin( ));
}

memory_block memory_block::shift_to_end(const memory_block& block) const
{
	return this->shift_to(block._Unchecked_end( ));
}

#pragma region flags_check

using flags_type = memory_block::flags_type;

// ReSharper disable once CppInconsistentNaming
class MEMORY_BASIC_INFORMATION_UPDATER : protected MEMORY_BASIC_INFORMATION
{
	static constexpr SIZE_T class_size = sizeof(MEMORY_BASIC_INFORMATION);

	template <typename Fn, typename ...Args>
	bool virtual_query(Fn&& native_function, Args&&...args)
	{
		return class_size == std::invoke(native_function, std::forward<Args>(args)..., static_cast<PMEMORY_BASIC_INFORMATION>(this), class_size);
	}

	//protected:
	//auto& get_flags( ) { return reinterpret_cast<flags_type&>(Protect); }
public:
	MEMORY_BASIC_INFORMATION_UPDATER( )
		: MEMORY_BASIC_INFORMATION( )
	{
	}

	auto& get_flags( ) const
	{
		return reinterpret_cast<const flags_type&>(Protect);
	}

	SIZE_T region_size( ) const
	{
		return this->RegionSize;
	}

	bool update(LPCVOID address)
	{
		return virtual_query(VirtualQuery, address);
	}

	bool update(HANDLE process, LPCVOID address)
	{
		return virtual_query(VirtualQueryEx, process, address);
	}
};

template <std::default_initializable Fn>
	requires(std::is_invocable_r_v<bool, Fn, flags_type, flags_type>)
class flags_checker : public MEMORY_BASIC_INFORMATION_UPDATER
{
	flags_checker(flags_type flags)
		: MEMORY_BASIC_INFORMATION_UPDATER( ),
		  flags_checked_(flags)
	{
	}

public:
	flags_checker(Fn&& checker_fn, flags_type flags)
		: flags_checker(flags)
	{
		checker_fn_ = std::move(checker_fn);
	}

	flags_checker(const Fn& checker_fn, flags_type flags)
		: flags_checker(flags)
	{
		checker_fn_ = checker_fn;
	}

private:
	Fn checker_fn_;
	flags_type flags_checked_;

public:
	std::optional<bool> operator()(SIZE_T block_size) const
	{
		//memory isn't commit!
		if (this->State != MEM_COMMIT)
			return false;
		//flags check isn't passed!
		if (std::invoke(checker_fn_, this->get_flags( ), flags_checked_) == false)
			return false;
		//found good result
		if (this->RegionSize >= block_size)
			return true;
		//check next block
		return {};
	}
};

template <typename Fn>
flags_checker(Fn&&) -> flags_checker<std::remove_cvref_t<Fn>>;

static constexpr flags_type _Page_read_flags    = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
static constexpr flags_type _Page_write_flags   = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOMBINE;
static constexpr flags_type _Page_execute_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

template <typename Fn>
static bool _Memory_block_flags_checker(flags_type flags, memory_block block, Fn&& checker_fn = {})
{
	flags_checker<Fn> checker(std::forward<Fn>(checker_fn), flags);
	for (;;)
	{
		if (!checker.update(block._Unchecked_begin( )))
			return false;
		auto result = checker(block.size( ));
		if (result.has_value( ))
			return *result;
		block = block.subblock(checker.region_size( ));
	}
}

struct have_flags_fn
{
	bool operator()(flags_type region_flags, flags_type target_flags) const
	{
		return region_flags & target_flags;
	}
};

struct dont_have_flags_fn
{
	bool operator()(flags_type region_flags, flags_type target_flags) const
	{
		return !(region_flags & target_flags);
	}
};

#pragma endregion

bool memory_block::have_flags(flags_type flags) const
{
	return _Memory_block_flags_checker<have_flags_fn>(flags, *this);
}

bool memory_block::dont_have_flags(flags_type flags) const
{
	return _Memory_block_flags_checker<dont_have_flags_fn>(flags, *this);
}

bool memory_block::readable( ) const
{
	return this->dont_have_flags(PAGE_NOACCESS);
}

bool memory_block::readable_ex( ) const
{
	return this->have_flags(_Page_read_flags);
}

bool memory_block::writable( ) const
{
	return this->have_flags(_Page_write_flags);
}

bool memory_block::executable( ) const
{
	return this->have_flags(_Page_execute_flags);
}

bool memory_block::code_padding( ) const
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
