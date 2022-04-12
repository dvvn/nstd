module;

#include <nstd/mem/block_includes.h>

#include <nstd/runtime_assert.h>
#include <nstd/ranges.h>

#include <algorithm>

module nstd.mem.block;
import nstd.mem.protect;

using namespace nstd;
using namespace nstd::mem;

block::block(const block_base span)
	: block_base(span)
{
}

static uint8_t* _Find_block_memchr(const size_t rng_size, const  size_t limit, uint8_t* const  start0, uint8_t* const  start2) noexcept
{
	size_t offset = 0;
	do
	{
		const auto start1 = static_cast<uint8_t* const>(std::memchr(start0 + offset, start2[0], limit - offset));
		if (!start1)
			break;

		if (std::memcmp(start1, start2, rng_size) == 0)
			return start1;

		offset = std::distance(start0, start1) + 1;
	}
	while (offset <= limit);

	return nullptr;
}

//10-100x slower than memchr version (debug)
static uint8_t* _Find_block_memcmp(const size_t rng_size, const  size_t limit, uint8_t* const  start0, uint8_t* const  start2) noexcept
{
	for (size_t offset = 0; offset < limit; ++offset)
	{
		const auto start1 = start0 + offset;
		if (std::memcmp(start1, start2, rng_size) == 0)
			return start1;
	}

	return nullptr;
}

//---

static block _Find_block_handmade(const block from, const block other) noexcept
{
	const auto rng_size = other.size( );
	const auto limit = from.size( ) - rng_size;

	const auto start0 = from.data( );
	const auto start2 = other.data( );

	if (rng_size == 1)
	{
		const auto found = std::memchr(start0, *start2, limit);
		if (found)
			return {static_cast<uint8_t* const>(found), 1};
	}
	else
	{
		const auto ptr = _Find_block_memchr(rng_size, limit, start0, start2);
		if (ptr)
			return {ptr,rng_size};
	}

	return {};
}

//2x slower than iters version (debug)
static block _Find_block_ranges(const block from, const block other) noexcept
{
	const auto found = std::ranges::search(from, other);
	if (!found.empty( ))
		return {found.data( ),found.size( )};
	return {};
	}

//3x slower than handmade verison (debug)
static block _Find_block_iters(const block from, const block other) noexcept
{
	const auto found = std::search(from.begin( ), from.end( ), other.begin( ), other.end( ));
	if (found != from.end( ))
		return {&*found,other.size( )};
	return {};
}

//---

block block::find_block(const block other) const noexcept
{
	return _Find_block_handmade(*this, other);
}

//~5x slower than modern version (debug)
static block _Find_unk_block(const block from, const signature_unknown_bytes& unkbytes) noexcept
{
	const auto _last_pos = from.data( ) + from.size( ) - unkbytes.bytes_count( );
	for (auto _pos = from.data( ); _pos <= _last_pos;)
	{
		auto inner_pos = _pos;
		for (const auto& [buff, skip] : unkbytes)
		{
			for (const auto chr : buff)
			{
				if (chr != *inner_pos++)
					goto _NO_RETURN;
			}
			inner_pos += skip;
		}

		return {_pos,inner_pos};

	_NO_RETURN:
		if (inner_pos == _pos)
			++_pos;
		else
			_pos = inner_pos;
	}

	return {};
}

static block _Find_unk_block_modern(const block from, const signature_unknown_bytes& unkbytes) noexcept
{
	const auto unkbytes_count = unkbytes.bytes_count( );
	if (from.size( ) < unkbytes_count)
		return {};

	const block unkbytes_first_block = {unkbytes[0].buff.begin( ), unkbytes[0].buff.end( )};

	if (unkbytes.size( ) == 1 && unkbytes.all_known( ))
		return from.find_block(unkbytes_first_block);

	const std::span unkbytes_except_first = {unkbytes.begin( ) + 1, unkbytes.end( )};
	const auto unkbytes_first_skip = unkbytes[0].skip;

	auto current_pos = from.data( );
	const auto last_pos = current_pos + from.size( );
	const auto last_readable_pos = last_pos - unkbytes_count;

	do
	{
		const block found0 = block(current_pos, last_pos).find_block(unkbytes_first_block);
		if (found0.empty( ))
			break;

		current_pos = found0.data( ) + found0.size( );
		auto tmp_pos = current_pos + unkbytes_first_skip;

		bool found = true;
		for (const auto& [buff, skip] : unkbytes_except_first)
		{
			const auto buff_size = buff.size( );
			if (std::memcmp(tmp_pos, buff.begin( ), buff_size) != 0)
			{
				found = false;
				break;
			}

			current_pos = tmp_pos + buff_size;
			tmp_pos = current_pos + skip;
		}

		if (found)
			return {found0.data( ), unkbytes_count};
	}
	while (current_pos <= last_readable_pos);

	return {};
}

block block::find_block(const signature_unknown_bytes& unkbytes) const noexcept
{
	return _Find_unk_block_modern(*this, unkbytes);
}

block block::shift_to(pointer ptr) const noexcept
{
	const auto offset = std::distance(this->data( ), ptr);
	const auto tmp = this->subspan(offset);
	return {tmp.data( ),tmp.size( )};
}
