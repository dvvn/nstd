module;

#include <span>

export module nstd.mem.block;
export import nstd.mem.signature;

using DWORD = unsigned long;

export namespace nstd::mem
{
    class block : public std::span<uint8_t>
    {
        using _Base = std::span<uint8_t>;

      public:
        block() = default;

        template <typename Bg>
        block(Bg* const begin, const size_type mem_size = sizeof(uintptr_t))
            : _Base((uint8_t*)begin, mem_size)
        {
            static_assert(sizeof(Bg) == sizeof(uint8_t));
        }

        template <typename Bg, typename Ed>
        block(Bg* const begin, Ed* const end)
            : _Base((uint8_t*)begin, (uint8_t*)end)
        {
            static_assert(sizeof(Bg) + sizeof(Ed) == sizeof(uint8_t) * 2);
        }

        explicit block(const _Base span);

        block find_block(const block other) const;
        block find_block(const unknown_signature& unkbytes) const;

        /*template <class StorageType>
        block find_block(const signature_known_bytes<StorageType>& rng) const
        {
            return find_block({rng.data( ),rng.size( )});
        }*/

        block shift_to(pointer ptr) const;
        // block subblock(size_t offset, size_t count = std::dynamic_extent) const;

        bool have_flags(DWORD flags) const;
        bool dont_have_flags(DWORD flags) const;
        DWORD get_flags() const;

        bool readable() const;
        bool readable_ex() const;
        bool writable() const;
        bool executable() const;
        bool code_padding() const;
    };
} // namespace nstd::mem
