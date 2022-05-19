module;

#include <span>

export module nstd.mem.block;
export import nstd.mem.signature;

using DWORD = unsigned long;

export namespace nstd::mem
{
    class block final : public std::span<uint8_t>
    {
        using _Base = std::span<uint8_t>;

      public:
        block() = default;

        template <typename Bg>
        block(Bg* const begin, const size_type mem_size = sizeof(uintptr_t)) : _Base((uint8_t*)begin, mem_size)
        {
            static_assert(sizeof(Bg) == sizeof(uint8_t));
        }
        template <typename Bg, typename Ed>
        block(Bg* const begin, Ed* const end) : _Base((uint8_t*)begin, (uint8_t*)end)
        {
            static_assert(sizeof(Bg) + sizeof(Ed) == sizeof(uint8_t) * 2);
        }

        explicit block(const _Base span);

        block find_block(const block other) const noexcept;
        block find_block(const signature_unknown_bytes& unkbytes) const noexcept;

        /*template <class StorageType>
        block find_block(const signature_known_bytes<StorageType>& rng) const
        {
            return find_block({rng.data( ),rng.size( )});
        }*/

        block shift_to(pointer ptr) const noexcept;
        // block subblock(size_t offset, size_t count = std::dynamic_extent) const;

        bool have_flags(DWORD flags) const noexcept;
        bool dont_have_flags(DWORD flags) const noexcept;
        DWORD get_flags() const noexcept;

        bool readable() const noexcept;
        bool readable_ex() const noexcept;
        bool writable() const noexcept;
        bool executable() const noexcept;
        bool code_padding() const noexcept;
    };
} // namespace nstd::mem
