module;

#include "block_includes.h"
#include <algorithm>

module nstd.mem.block;
import nstd.mem.protect;

using namespace nstd;
using namespace nstd::mem;

//block::block(uint8_t* begin, size_type mem_size)
//	: block_base(begin.pointer.get( ), mem_size)
//{
//}
//
//block::block(uint8_t* begin, uint8_t* end)
//	: block_base(begin.pointer.get( ), end.pointer.get( ))
//{
//}

block::block(const block_base& span)
	: block_base(span)
{
}

block block::find_block(const block other) const
{
	const auto rng_size = other.size( );
	const auto limit = this->size( ) - rng_size;

	const auto start0 = this->data( );
	const auto start2 = other.data( );

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

static unknown_find_result _Find_unknown_bytes(const block& mem, const signature_unknown_bytes& unkbytes)
{
	size_t offset = 0;
	size_t scanned = 0;

	const auto& [known0, skip0] = unkbytes[0];
	if (!known0.empty( ))
	{
		// ReSharper disable once CppInconsistentNaming
		const auto _Begin = mem.find_block({known0.data( ), known0.size( )});
		if (_Begin.empty( ))
			return {mem.size( ) - known0.size( ), 0, false};
		scanned = std::distance(mem.data( ), _Begin.data( ));
		offset = _Begin.size( );
	}

	offset += skip0;

	for (const auto& [known, skip] : std::span(std::next(unkbytes.begin( )), unkbytes.end( )))
	{
		// ReSharper disable once CppInconsistentNaming
		const auto _Next = mem.subblock(scanned + offset);
		const auto start1 = _Next.data( );
		const auto start2 = known.data( );
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

		for (const auto& [known, skip] : rng)
		{
			size += known.size( );
			size += skip;
		}

		return size;
	}();

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
	const auto offset = std::distance(this->data( ), ptr);
	return this->subblock(offset);
}
