#include "block.h"
#include "protect.h"

#include "nstd/runtime_assert_fwd.h"
#include "nstd/signature.h"
#include "nstd/address.h"

#include <algorithm>

using namespace nstd;
using namespace nstd::mem;

block::block(const address& begin, size_type mem_size)
	: storage_type(begin.ptr<uint8_t>( ), mem_size)
{
}

block::block(const address& begin, const address& end)
	: storage_type(begin.ptr<uint8_t>( ), end.ptr<uint8_t>( ))
{
}

block::block(const address& addr)
	: block(addr, sizeof(address))
{
}

block::block(const storage_type& span)
	: storage_type(span)
{
}

block block::find_block(std::span<const uint8_t> unkbytes) const
{
	const auto rng_size = unkbytes.size( );
	const auto limit    = this->size( ) - rng_size;

	const auto start0 = this->_Unchecked_begin( );
	const auto start2 = unkbytes._Unchecked_begin( );

	for (auto offset = static_cast<size_t>(0); offset < limit; ++offset)
	{
		const auto start1 = start0 + offset;
		if (std::memcmp(start1, start2, rng_size) == 0)
			return {start1, rng_size};
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

static unknown_find_result _Find_unknown_bytes(const block& block, const signature_unknown_bytes& unkbytes)
{
	size_t offset  = 0;
	size_t scanned = 0;


	const auto& [known0, skip0] = unkbytes[0];
	if (!known0.empty( ))
	{
		// ReSharper disable once CppInconsistentNaming
		const auto _Begin = block.find_block(known0);
		if (_Begin.empty( ))
			return {block.size( ) - known0.size( ), 0, false};
		scanned = std::distance(block._Unchecked_begin( ), _Begin._Unchecked_begin( ));
		offset  = _Begin.size( );
	}

	offset += skip0;

	for (const auto& [known, skip]: std::span(std::next(unkbytes.begin( )), unkbytes.end( )))
	{
		// ReSharper disable once CppInconsistentNaming
		const auto _Next = block.subblock(scanned + offset);
		const auto start1 = _Next._Unchecked_begin( );
		const auto start2 = known._Unchecked_begin( );
		if (std::memcmp(start1, start2, known.size( )) != 0)
			return {scanned, offset, false};

		offset += known.size( ) + skip;
	}

	return {scanned, offset, true};
}

block block::find_block(const signature_unknown_bytes& rng) const
{
	const auto rng_size = [&]
	{
		size_t size = 0;

		for (const auto& [known, skip]: rng)
		{
			size += known.size( );
			size += skip;
		}

		return size;
	}( );

	auto block = *this;

	while (block.size( ) >= rng_size)
	{
		const auto [scanned, found] = _Find_unknown_bytes(block, rng);
		if (found)
		{
			runtime_assert(scanned.after == rng_size);
			return block.subblock(scanned.before, rng_size);
		}

		block = block.subblock(scanned.before + 1);
	}

	return {};
}

block block::subblock(size_t offset, size_t count) const
{
	return block(this->subspan(offset, count));
}

block block::shift_to(pointer ptr) const
{
	const auto offset = std::distance(this->_Unchecked_begin( ), ptr);
	return this->subblock(offset);
}
