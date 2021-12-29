module;

#include <span>

export module nstd.mem.block;

export namespace nstd
{
	class address;
	class signature_unknown_bytes;
	template <class StorageType>
	class signature_known_bytes;
}

export namespace nstd::mem
{
	class block final : std::span<uint8_t>
	{
	public:
		using storage_type = std::span<uint8_t>;

		using storage_type::begin;
		using storage_type::end;
		using storage_type::_Unchecked_begin;
		using storage_type::_Unchecked_end;

		using storage_type::size;
		using storage_type::empty;
		using storage_type::operator[];

		block( ) = default;

		block(const address& begin, size_type mem_size);
		block(const address& begin, const address& end);
		block(const address& addr);

		explicit block(const storage_type& span);

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
