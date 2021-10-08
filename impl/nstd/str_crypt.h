#pragma once
#include "xor_shift.h"

#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/for_each.hpp>

//#define STR_CRYPTOR_MUTABLE

#if __cpp_nontype_template_args >= 201911
#pragma message ("__cpp_nontype_template_args updated")//https://en.cppreference.com/w/cpp/language/user_literal
#endif

namespace cheat::utl
{
    namespace detail
    {
        template <typename T>
        concept character = !std::is_same_v<typename std::char_traits<T>::int_type, long>;
        static_assert(std::is_same_v<std::char_traits<void*>::int_type, long>, " std::char_traits<unknown>::int_type changed!");

#pragma pack(push, 1)

        template <character Ch, size_t Str_length, size_t PadBefore, size_t PadAfter>
        class str_cryptor_cache
        {
            std::array<Ch, PadBefore + Str_length + PadAfter + 1> cache_ = { };

        protected:
            constexpr void setup(const Ch (&str)[Str_length])
            {
                ranges::copy(str, this->get_text( ).data( ));
                cache_.back( ) = '\0'; //fake null-terminated string

                uint64_t state{__time__.seconds.count( ) + 1};
                const auto crypt_dummy = [&](Ch& c)
                {
                    c = xor_shift::xorshift::xorshift64(state);
                };
                ranges::for_each(this->get_pad_before( ), crypt_dummy);
                ranges::for_each(this->get_pad_after( ), crypt_dummy);
            }

        private:
            //dont abuse std::ranges. they fuck ur pc

            template <typename T>
            static constexpr auto Get_text_impl(T&& cache)
            {
                //auto dropped = std::views::drop(cache, Pad_before);
                //auto taken   = std::views::take(dropped, Str_length);
                //return taken;

                auto data = cache.data( ) + PadBefore;
                return std::span(data, Str_length);
            }

            template <typename T>
            static constexpr auto Get_pad_before_impl(T&& cache)
            {
                //auto taken = std::views::take(cache, Pad_before);
                //return taken;

                auto data = cache.data( );
                return std::span(data, PadBefore);
            }

            template <typename T>
            static constexpr auto Get_pad_after_impl(T&& cache)
            {
                //return cache_ | std::views::drop(Pad_before + Str_length) | std::views::take(Pad_after);

                //auto dropped = std::views::drop(cache, Pad_before + Str_length);
                //auto taken   = std::views::take(dropped, Pad_after);
                //return taken;

                auto data = cache.data( ) + (PadBefore + Str_length);
                return std::span(data, PadAfter);
            }

            template <typename T>
            static constexpr auto Get_whole_cache_impl(T&& cache)
            {
                //auto taken = std::views::take(cache, Pad_before + Str_length + _pad_size2);
                //return taken;

                auto data = cache.data( );
                return std::span(data, PadBefore + Str_length + PadAfter);
            }

        protected:
            constexpr _NODISCARD auto get_text( ) { return Get_text_impl(cache_); }
            constexpr _NODISCARD auto get_pad_before( ) { return Get_pad_before_impl(cache_); }
            constexpr _NODISCARD auto get_pad_after( ) { return Get_pad_after_impl(cache_); }
            constexpr _NODISCARD auto get_whole_cache( ) { return Get_whole_cache_impl(cache_); }

        public:
            constexpr _NODISCARD auto get_text( ) const { return Get_text_impl(cache_); }
            constexpr _NODISCARD auto get_pad_before( ) const { return Get_pad_before_impl(cache_); }
            constexpr _NODISCARD auto get_pad_after( ) const { return Get_pad_after_impl(cache_); }
            constexpr _NODISCARD auto get_whole_cache( ) const { return Get_whole_cache_impl(cache_); }
        };

        constexpr size_t str_crypt_pad_size(const size_t char_size, const size_t magic, const size_t limit)
        {
#ifdef UTILS_X32
            const auto shift_fn = xor_shift::xorshift::xorshift32_;
#else
            const auto shift_fn = xor_shift::xorshift::xorshift64_;
#endif

            const auto val = shift_fn(magic) % (limit / char_size);
            if (val == 0)
                return char_size + 1;
            return val;
        }

        template <character Ch>
        class str_crypt_fn
        {
        public:
            constexpr str_crypt_fn(size_t state) : state_(state) {}

            constexpr void operator()(Ch& c)
            {
                size_t magic;
                if constexpr (sizeof(size_t) == sizeof(uint32_t))
                    magic = xor_shift::xorshift::xorshift32(state_);
                else
                    magic = xor_shift::xorshift::xorshift64(state_);

                c ^= magic;
            }

        private:
            size_t state_;
        };

        template <character Ch, size_t Str_length>
        class str_cryptor: public str_cryptor_cache<Ch, Str_length,
                                                     str_crypt_pad_size(sizeof(Ch), __time__.seconds.count( ), 128) + 1,
                                                     str_crypt_pad_size(sizeof(Ch), __time__.hours.count( ), 128) + 1>
        {
        public:
            constexpr str_cryptor(const Ch (&str)[Str_length], str_crypt_fn<Ch> crypt, str_crypt_fn<Ch> decrypt) :
                crypt_(std::std::move(crypt)), decrypt_(std::std::move(decrypt))
            {
                this->setup(str);
            }

            constexpr bool crypt( )
            {
                if (crypted_)
                    return false;

                if (!precompile_.has_value( ))
                    precompile_ = std::is_constant_evaluated( );

                Apply_fn(crypt_);
                crypted_ = true;

                return true;
            }

            /*_NODISCARD constexpr auto crypt_copy( ) const
            {
                _Str_cryptor copy = *this;
                copy.crypt( );
                return copy;
            }*/

            constexpr bool decrypt( )
            {
                if (!crypted_)
                    return false;

                Apply_fn(decrypt_);
                crypted_ = false;

                return true;
            }

            /*_NODISCARD constexpr _Str_cryptor decrypt_copy( ) const
            {
                _Str_cryptor copy = *this;
                copy.decrypt( );
                return copy;
            }*/

        private:
            /**
             * \brief 
             * \tparam Store_as_array : true - store to array, false - store to std::string
             */
            template <bool Store_as_array>

            struct Unwrap_text_fn;

            // ReSharper disable CppExplicitSpecializationInNonNamespaceScope

            template < >
            struct Unwrap_text_fn<true>
            {
                template <typename T>
                std::array<Ch, Str_length> operator()(T&& text) const
                {
                    std::array<Ch, Str_length> buffer{ };
                    ranges::copy(text, buffer.begin( ));
                    buffer.back( ) = '\0'; //force last char to 0 if we return crypted string
                    return buffer;
                }
            };

            template < >
            struct Unwrap_text_fn<false>
            {
                template <typename T>
                std::basic_string<Ch> operator()(T&& text) const
                {
                    return (text);
                }
            };

            // ReSharper restore CppExplicitSpecializationInNonNamespaceScope

        public:
            template <bool Store_as_array = true>
            _NODISCARD auto unwrap_text(bool decrypt = true) const
            {
                static constexpr auto unwrap_text = Unwrap_text_fn<Store_as_array>( );
                if (decrypt && crypted_)
                {
                    str_cryptor copy = *this;
                    copy.decrypt( );
                    return unwrap_text(copy.get_text( ));
                }

                return unwrap_text(this->get_text( ));
            }

            template <bool Store_as_array = true>
            _NODISCARD auto unwrap_text(bool decrypt = true, bool self = true)
            {
                if (decrypt && self && crypted_)
                    this->decrypt( );

                const str_cryptor& cthis = *this;
                return cthis.unwrap_text<Store_as_array>(decrypt);
            }

        private:
            template <typename Fn>
            constexpr void Apply_fn(Fn&& fn)
            {
                if (*precompile_)
                    ranges::for_each(this->get_whole_cache( ), fn);
                else
                    ranges::for_each(this->get_text( ), fn);
            }

            bool crypted_ = false;

            std::std::optional<bool> precompile_;

            str_crypt_fn<Ch> crypt_;
            str_crypt_fn<Ch> decrypt_;
        };

#pragma pack(pop)

        struct str_cryptor_fn
        {
            template <character Ch, size_t Str_length>
            constexpr str_cryptor<Ch, Str_length> operator()(const Ch (&str)[Str_length], const bool crypt = true, size_t magic_add = 0) const
            {
                auto str_hash = Str_length;//hash::fnv1a(str);
                constexpr auto time_val = static_cast<size_t>(__time__.seconds.count( ) + __time__.hours.count( ));

                auto state = str_hash + magic_add + time_val;

                auto fn = str_crypt_fn<Ch>(state);
                auto result = str_cryptor<Ch, Str_length>(str, fn, fn);
                if (crypt)
                    result.crypt( );
                return result;
            }
        };
    }

    inline static constexpr auto crypt_str = detail::str_cryptor_fn( );
}

#ifndef _DEBUG
#define CRYPT_STR(str) ( []{\
        constexpr auto result = cheat::tools::to_string_view_ex(str);\
        return result;\
    }() )
#else
#define CRYPT_STR(str) ( []()->auto&{\
        static constexpr auto crypted = cheat::tools::crypt_str(str/*, cheat::tools::hash::fnv1a(__FILE__)+__LINE__*/);\
        static const auto decrypted   = crypted.unwrap_text();\
        static const auto result      = std::basic_string_view(decrypted.data(),decrypted.size()-1);\
        return result;\
    }() )
#endif


//#define CRYPT_STR_COPY(str) ::cheat::tools::to_string_ex(CRYPT_STR(str))
//#define CRYPT_STR_PTR(str)  CRYPT_STR(str).data()

//inline auto aa = []( )-> auto&
//{
//    static constexpr auto crypted = cheat::tools::crypt_str("aaa");
//    static const auto decrypted   = crypted.decrypt( );
//    static const auto result      = cheat::tools::any_string_view(decrypted);
//    return result;
//}( );
