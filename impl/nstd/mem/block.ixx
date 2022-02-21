module;

#include "block_includes.h"

export module nstd.mem.block;
//export import :address;
export import nstd.mem.signature;

export namespace nstd::inline mem
{
	using block_base = std::span<uint8_t>;
	class block final :public block_base
	{
	public:

		/*using block_base::begin;
		using block_base::end;
		using block_base::_Unchecked_begin;
		using block_base::_Unchecked_end;

		using block_base::iterator;
		using block_base::value_type;

		using block_base::size;
		using block_base::empty;
		using block_base::operator[];*/

		block( ) = default;

		//using address_type = basic_address<uint8_t>; 

		block(uint8_t* begin, size_type mem_size = sizeof(uintptr_t))
			:block_base(begin, mem_size)
		{
		}
		block(uint8_t* begin, uint8_t* end)
			: block_base(begin, end)
		{
		}

		explicit block(const block_base& span);

		block find_block(std::span<const uint8_t> rng) const;
		block find_block(const signature_unknown_bytes& rng) const;

		template <class StorageType>
		block find_block(const signature_known_bytes<StorageType>& rng) const { return find_block(rng.storage( )); }

		block shift_to(pointer ptr) const;
		block subblock(size_t offset, size_t count = std::dynamic_extent) const;

#ifdef NSTD_MEM_BLOCK_CHECK_CUSTOM_FLAGS
		bool have_flags(DWORD flags) const;
		bool dont_have_flags(DWORD flags) const;
#endif 
		bool readable( ) const;
		bool readable_ex( ) const;
		bool writable( ) const;
		bool executable( ) const;
		bool code_padding( ) const;
	};
}
