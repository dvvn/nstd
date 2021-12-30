module;

#include "block_includes.h"

export module nstd.mem.block;
export import nstd.address;
export import nstd.signature;

export namespace nstd::mem
{
	using block_base = std::span<uint8_t>;
	class block final : block_base
	{
	public:

		using block_base::begin;
		using block_base::end;
		using block_base::_Unchecked_begin;
		using block_base::_Unchecked_end;

		using block_base::size;
		using block_base::empty;
		using block_base::operator[];

		block( ) = default;

		block(const address& begin, size_type mem_size);
		block(const address& begin, const address& end);
		block(const address& addr);

		explicit block(const block_base& span);

		block find_block(std::span<const uint8_t> rng) const;
		block find_block(const signature_unknown_bytes& rng) const;

		template <class StorageType>
		block find_block(const signature_known_bytes<StorageType>& rng) const { return find_block(rng.storage( )); }

		block shift_to(pointer ptr) const;
		block subblock(size_t offset, size_t count = std::dynamic_extent) const;

		using flags_type = unsigned long;

		bool have_flags(flags_type flags) const;
		bool dont_have_flags(flags_type flags) const;

		bool readable( ) const;
		bool readable_ex( ) const;
		bool writable( ) const;
		bool executable( ) const;
		bool code_padding( ) const;
	};
}
