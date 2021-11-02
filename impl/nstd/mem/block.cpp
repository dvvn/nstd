#include "block.h"
#include "protect.h"

#include "nstd/runtime_assert_fwd.h"
#include "nstd/signature.h"
#include "nstd/address.h"

#include <algorithm>
#include <optional>

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

block block::find_block(std::span<const uint8_t> rng) const
{
	const auto rng_size = rng.size( );
	const auto limit    = this->size( ) - rng_size;

	const auto start1 = this->_Unchecked_begin( );
	const auto start2 = rng._Unchecked_begin( );

	for (auto offset = static_cast<size_t>(0); offset < limit; ++offset)
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

static unknown_find_result _Find_unknown_bytes(const block& block, const signature_unknown_bytes& rng)
{
	size_t offset  = 0;
	size_t scanned = 0;

	const auto& storage = rng.storage( );
	if (!storage.empty( ))
	{
		const auto found = block.find_block(storage);
		if (found.empty( ))
			return {block.size( ) - storage.size( ), 0, false};
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

block block::find_block(const signature_unknown_bytes& rng) const
{
	const auto rng_size = [&]
	{
		size_t size = 0;
		for (auto rng0 = std::addressof(rng); rng0 != nullptr; rng0 = rng0->next.get( ))
		{
			size += rng0->storage( ).size( );
			size += rng0->skip;
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


