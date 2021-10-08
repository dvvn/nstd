#include "memory block.h"

#include <windows.h>
static_assert(std::same_as<unsigned long, DWORD>);

using namespace nstd;

bool any_bytes_range::known( ) const
{
	return std::visit(overload
					  (
					   [](const known_bytes_range_const&  ) { return true; },
					   [](const unknown_bytes_range_const&) { return false; }
					  ), data_);
#if 0
	if(known)
	{
		runtime_assert(try_rewrap_ == false);
	}
	else if(try_rewrap_)
	{
		try_rewrap_ = false;
		const auto rng = this->get_unknown( );
		if(all_bytes_known(rng))
		{
			auto rng2 = make_bytes_known(rng);
			this->data_.emplace<known_bytes_object>(std::move(rng2));
			return true;
		}
	}

	return known;
#endif
}

known_bytes_range_const any_bytes_range::get_known( ) const
{
	return std::visit(overload([](const known_bytes_range_const& obj)
							   {
								   return obj;
							   },
							   [](const unknown_bytes_range_const&)
							   {
								   runtime_assert("incorrect call");
								   return known_bytes_range_const( );
							   }), data_);
}

unknown_bytes_range_const any_bytes_range::get_unknown( ) const
{
	return std::visit(overload([](const known_bytes_range_const&)
							   {
								   runtime_assert("incorrect call");
								   return unknown_bytes_range_const( );
							   },
							   [](const unknown_bytes_range_const& obj)
							   {
								   return obj;
							   }), data_);
}

memory_block::memory_block(const address& begin, size_type mem_size)
	: bytes_(begin.ptr<known_byte>( ), mem_size)
{
}

memory_block::memory_block(const address& begin, const address& end)
	: bytes_(begin.ptr<known_byte>( ), end.ptr<known_byte>( ))
{
}

memory_block::memory_block(const address& addr)
	: memory_block(addr, sizeof(address))
{
}

memory_block::memory_block(const known_bytes_range& span)
	: bytes_(span)
{
}

struct rewrap_range_exception final: std::exception
{
};

template <typename Where, typename What, typename Pr = std::ranges::equal_to>
static auto _Rng_search(Where&& where, What&& what, Pr&& pred = { })
{
	//ranges extremely slow in debug mode
	auto a = std::initializer_list(std::_Get_unwrapped(where.begin( )), std::_Get_unwrapped(where.end( )));
	auto b = std::initializer_list(std::_Get_unwrapped(what.begin( )), std::_Get_unwrapped(what.end( )));
#if /*_ITERATOR_DEBUG_LEVEL == 0 && !defined(_DEBUG)*/ 0

	return std::ranges::search(a, b, pred);

#else
	auto first     = a.begin( );
	auto real_last = a.end( );
	auto last      = real_last - what.size( );
	for (auto itr = first; itr != last; ++itr)
	{
		auto itr_temp = itr;
		auto found    = true;
		// ReSharper disable CppInconsistentNaming
		for (auto&& _Left: what)
		{
			auto& _Right = *itr_temp++;
			if (!pred(_Right, _Left))
			{
				found = false;
				break;
			}
		}
		// ReSharper restore CppInconsistentNaming
		if (found)
			return std::ranges::subrange{itr, itr + what.size( )};
	}
	return std::ranges::subrange{real_last, real_last};
#endif
}

template <typename T, typename Span = std::span<const T>, typename Ptr = const T*>
static std::optional<Span> _Rewrap_range(const known_bytes_range_const& rng)
{
	const auto size_bytes = rng.size( );
	if (size_bytes < sizeof(T))
		throw rewrap_range_exception( );
	if (size_bytes == sizeof(T))
		return Span(reinterpret_cast<Ptr>(rng._Unchecked_begin( )), 1);
	const auto tail = size_bytes % sizeof(T);
	if (tail > 0)
		return { };
	auto start = reinterpret_cast<Ptr>(rng._Unchecked_begin( ));
	auto size  = size_bytes / sizeof(T);
	return Span(start, size);
}

template <typename T>
static memory_block_opt _Scan_memory(const known_bytes_range& block, const std::span<T>& rng)
{
	const auto unreachable = block.size( ) % rng.size_bytes( );
	const auto fake_block  = std::span<T>(reinterpret_cast<T*>(block._Unchecked_begin( )), (block.size( ) - unreachable) / sizeof(T));
	auto       result      = _Rng_search(fake_block, rng);
	if (result.empty( ))
		return { };
	return memory_block({reinterpret_cast<known_byte*>(result.begin( )), rng.size_bytes( )});
}

static memory_block_opt _Scan_memory(const known_bytes_range& block, const known_bytes_range_const& data)
{
	auto result = _Rng_search(block, data);
	if (result.empty( ))
		return { };
	return memory_block({result.begin( ), data.size( )});
}

memory_block_opt memory_block::find_block_impl(const known_bytes_range_const& rng) const
{
	//doen't work sometimes, idk why
	/*try
	{
		if (const auto rng64 = _Rewrap_range<uint64_t>(rng); rng64.has_value( ))
			return _Scan_memory(bytes_, *rng64);
		if (const auto rng32 = _Rewrap_range<uint32_t>(rng); rng32.has_value( ))
			return _Scan_memory(bytes_, *rng32);
		if (const auto rng16 = _Rewrap_range<uint16_t>(rng); rng16.has_value( ))
			return _Scan_memory(bytes_, *rng16);
	}
	catch (const rewrap_range_exception&)
	{
	}*/

	return _Scan_memory(bytes_, rng);
}

memory_block_opt memory_block::find_block_impl(const unknown_bytes_range_const& rng) const
{
#ifdef NSTD_MEM_BLOCK_UNWRAP_UNKNOWN_BYTES
	if (all_bytes_known(rng))
	{
		return this->find_block_impl(make_bytes_known(rng));
	}
#else
	runtime_assert(!all_bytes_known(rng), "Unknown bytes must be unwrapped before!");
#endif
	auto result = _Rng_search(bytes_, rng, [](const known_byte kbyte, const unknown_byte& unkbyte)
	{
		return !unkbyte.has_value( ) || *unkbyte == kbyte;
	});
	if (result.empty( ))
		return { };
	return memory_block({(result.begin( )), rng.size( )});
}

memory_block_opt memory_block::find_block_impl(const any_bytes_range& rng) const
{
	memory_block_opt ret;
	if (rng.known( ))
		ret = this->find_block_impl(rng.get_known( ));
	else
		ret = this->find_block_impl(rng.get_unknown( ));
	return ret;
}

address memory_block::addr( ) const
{
	return bytes_._Unchecked_begin( );
}

address memory_block::last_addr( ) const
{
	return bytes_._Unchecked_end( );
}

memory_block memory_block::subblock(size_t offset) const
{
	return memory_block(bytes_.subspan(offset));
}

memory_block memory_block::shift_to(pointer ptr) const
{
	const auto offset = std::distance(bytes_._Unchecked_begin( ), ptr);
	return this->subblock(offset);
}

memory_block memory_block::shift_to_start(const memory_block& block) const
{
	return this->shift_to(block.bytes_._Unchecked_begin( ));
}

memory_block memory_block::shift_to_end(const memory_block& block) const
{
	return this->shift_to(block.bytes_._Unchecked_end( ));
}

#pragma region flags_check

using flags_type = memory_block::flags_type;

// ReSharper disable once CppInconsistentNaming
class MEMORY_BASIC_INFORMATION_UPDATER: protected MEMORY_BASIC_INFORMATION
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
class flags_checker: public MEMORY_BASIC_INFORMATION_UPDATER
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
	Fn         checker_fn_;
	flags_type flags_checked_;

public:
	std::optional<bool> check_flags(SIZE_T block_size) const
	{
		//memory isnt commit!
		if (this->State != MEM_COMMIT)
			return false;
		//flags check isnt passed!
		if (std::invoke(checker_fn_, this->get_flags( ), flags_checked_) == false)
			return false;
		//found good result
		if (this->RegionSize >= block_size)
			return true;
		//check next block
		return { };
	}
};

template <typename Fn>
flags_checker(Fn&&) -> flags_checker<std::remove_cvref_t<Fn>>;

static constexpr flags_type _Page_read_flags    = (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE);
static constexpr flags_type _Page_write_flags   = (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOMBINE);
static constexpr flags_type _Page_execute_flags = (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);

template <typename Fn>
static bool _Memory_block_flags_checker(flags_type flags, memory_block block, Fn&& checker_fn = { })
{
	flags_checker<Fn> checker(std::forward<Fn>(checker_fn), flags);
	while (true)
	{
		auto& bytes_rng = block.bytes_range( );
		if (!checker.update(bytes_rng._Unchecked_begin( )))
			return false;
		auto result = checker.check_flags(bytes_rng.size( ));
		if (result.has_value( ))
			return *result;
		block = block.subblock(checker.region_size( ));
	}
}

struct have_flags_fn
{
	bool operator()(flags_type region_flags, flags_type target_flags) const
	{
		return region_flags & (target_flags);
	}
};

struct dont_have_flags_fn
{
	bool operator()(flags_type region_flags, flags_type target_flags) const
	{
		return !(region_flags & (target_flags));
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
	const auto first = bytes_.front( );
	if (first != 0x00 && first != 0x90 && first != 0xCC)
		return false;
	for (const auto val: this->subblock(1).bytes_)
	{
		if (val != first)
			return false;
	}
	return true;
}

const known_bytes_range& memory_block::bytes_range( ) const
{
	return bytes_;
}
