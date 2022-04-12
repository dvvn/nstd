module;

#include "block_includes.h"

export module nstd.mem.block;
//export import :address;
export import nstd.mem.signature;

export namespace nstd::mem
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

		template<typename Bg>
		block(Bg* begin, size_type mem_size = sizeof(uintptr_t))
			:block_base((uint8_t*)begin, mem_size)
		{
			static_assert(sizeof(Bg) == sizeof(uint8_t));
		}
		template<typename Bg, typename Ed>
		block(Bg* begin, Ed* end)
			: block_base((uint8_t*)begin, (uint8_t*)end)
		{
			static_assert(sizeof(Bg) + sizeof(Ed) == sizeof(uint8_t) * 2);
		}

		explicit block(const block_base span);

		block find_block(const block other) const noexcept;
		block find_block(const signature_unknown_bytes& unkbytes) const noexcept;

		/*template <class StorageType>
		block find_block(const signature_known_bytes<StorageType>& rng) const
		{
			return find_block({rng.data( ),rng.size( )});
		}*/

		block shift_to(pointer ptr) const noexcept;
		//block subblock(size_t offset, size_t count = std::dynamic_extent) const;

#ifdef NSTD_MEM_BLOCK_CHECK_CUSTOM_FLAGS
		bool have_flags(DWORD flags) const;
		bool dont_have_flags(DWORD flags) const;
		DWORD get_flags( ) const;
#endif 
		bool readable( ) const noexcept;
		bool readable_ex( ) const noexcept;
		bool writable( ) const noexcept;
		bool executable( ) const noexcept;
		bool code_padding( ) const noexcept;
	};
}
