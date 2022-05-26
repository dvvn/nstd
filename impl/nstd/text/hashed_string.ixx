module;

#include <nstd/core_utils.h>
#include <nstd/runtime_assert.h>

#include <string>

#ifdef _DEBUG
#include <stdexcept>
#endif

export module nstd.text.hashed_string;
import nstd.lazy_invoke;
import nstd.type_name;

#define COMMA ,

#define CALL_ORIGINAL(_NAME_) Base::_NAME_(std::forward<Args>(args)...)

#define WRAP(_RET_, _NAME_, _PROXY_, ...)                           \
  private:                                                          \
    using Base::_NAME_;                                             \
                                                                    \
  public:                                                           \
    template <typename... Args>                                     \
    _RET_ _NAME_(Args&&... args)                                    \
    {                                                               \
        const nstd::lazy_invoke lazy = [=] { this->_Calc_hash(); }; \
        _PROXY_ CALL_ORIGINAL(_NAME_);                              \
        __VA_ARGS__;                                                \
    }

#define WRAP_VOID(_NAME_) WRAP(void, _NAME_, )

#define WRAP_THIS(_NAME_) WRAP(auto&, _NAME_, , return *this)

template <class Base, typename T, typename Q>
decltype(auto) this_or_iterator(T* thisptr, const Q& proxy_result)
{
    if constexpr (std::same_as<Q, Base>)
    {
        // return *this instead of *Base
        return *thisptr;
    }
    else
    {
        // return the iterator
        return proxy_result;
    }
}

#define WRAP_THIS_OR_ITERATOR(_NAME_) WRAP(decltype(auto), _NAME_, decltype(auto) result =, return this_or_iterator<Base>(this COMMA result))

template <class Base, template <typename> class Hasher = std::hash>
struct hashed_string_wrapper : Base
{
    using hash_type = /*decltype(std::invoke(std::declval<Hasher>( ), std::declval<Base>( )))*/ size_t;
    using hash_func_type = Hasher<Base>;

    static_assert(std::is_empty_v<hash_func_type>, "Hasher class must be empty");

  private:
    [[no_unique_address]] hash_func_type hasher_;
    hash_type hash_;

    constexpr hash_type _Get_hash() const
    {
        return std::invoke(hasher_, *static_cast<const Base*>(this));
    }

    constexpr void _Validate_hash(hash_type hash) const
    {
#ifdef _DEBUG
        if (_Get_hash() == hash)
            return;
        if (std::is_constant_evaluated())
            throw std::logic_error("Incorrect hash passed");
        else
            runtime_assert("Incorrect hash passed");
#endif
    }

    constexpr void _Calc_hash()
    {
        hash_ = _Get_hash();
    }

    constexpr void _Write_hash(hash_type hash)
    {
        _Validate_hash(hash);
        hash_ = hash;
    }

    template <class Base2, template <typename> class Hasher2>
    constexpr void _Try_write_hash(const hashed_string_wrapper<Base2, Hasher2>& holder)
    {
        static_assert(std::constructible_from<Base, Base2>);

        if constexpr (!same_template<Hasher, Hasher2>())
        {
            _Calc_hash();
        }
        else
        {
            using val1_t = decltype(std::declval<Base>()[0]);
            using val2_t = decltype(std::declval<Base2>()[0]);

            if constexpr (std::same_as<val1_t, val2_t>)
                hash_ = holder.hash_;
            else
                _Write_hash(holder.hash_);
        }
    }

  public:
    constexpr hashed_string_wrapper()
    {
        _Calc_hash();
    }

    template <class Base2, template <typename> class Hasher2>
    requires(std::constructible_from<Base, Base2>) constexpr hashed_string_wrapper(const hashed_string_wrapper<Base2, Hasher2>& other) : Base(static_cast<const Base2&>(other))
    {
        _Try_write_hash(other);
    }

    template <class Base2, template <typename> class Hasher2>
    requires(std::constructible_from<Base, Base2>) constexpr hashed_string_wrapper(hashed_string_wrapper<Base2, Hasher2>&& other) : Base(static_cast<Base2&&>(other))
    {
        _Try_write_hash(other);
    }

    template <class Base2>
    requires(std::constructible_from<Base, Base2>) constexpr hashed_string_wrapper(Base2&& other) : Base(std::forward<Base2>(other))
    {
        _Calc_hash();
    }

    template <class Base2>
    requires(std::constructible_from<Base, Base2>) constexpr hashed_string_wrapper(Base2&& other, hash_type hash) : Base(std::forward<Base2>(other))
    {
        _Write_hash(hash);
    }

    template <typename Itr>
    constexpr hashed_string_wrapper(Itr bg, Itr ed) : Base(bg, ed)
    {
        _Calc_hash();
    }

    template <typename Itr>
    constexpr hashed_string_wrapper(Itr bg, Itr ed, hash_type hash) : Base(bg, ed)
    {
        _Write_hash(hash);
    }

    constexpr hash_func_type hash_function() const
    {
        return hasher_;
    }
    constexpr hash_type hash() const
    {
        return hash_;
    }
};

template <typename Chr, template <typename> class Hasher = std::hash, class Traits = std::char_traits<Chr>, class Base = hashed_string_wrapper<std::basic_string_view<Chr, Traits>, Hasher>>
struct basic_hashed_string_view : Base
{
    using Base::Base;

    WRAP_THIS(substr);

    WRAP_VOID(remove_prefix);
    WRAP_VOID(remove_suffix);

  private:
#if __cplusplus > 201703L
    using Base::swap;
#endif
};

template <typename Chr, template <typename> class Hasher = std::hash, class Traits = std::char_traits<Chr>, class Allocator = std::allocator<Chr>, class Base = hashed_string_wrapper<std::basic_string<Chr, Traits, Allocator>, Hasher>>
struct basic_hashed_string : Base
{
    using Base::Base;

    WRAP_THIS(substr);

    WRAP_THIS(assign);
    WRAP_VOID(clear);
    WRAP_THIS_OR_ITERATOR(insert);
    WRAP_THIS_OR_ITERATOR(erase);
    WRAP_VOID(push_back);
    WRAP_VOID(pop_back);
    WRAP_THIS(append);
    WRAP_THIS(operator+=);
    WRAP_THIS(replace);
    WRAP_VOID(resize);
#ifdef __cpp_lib_string_resize_and_overwrite
    WRAP_VOID(resize_and_overwrite);
#endif

  private:
    using Base::swap;
};

#define DECLARE_IMPL(_TYPE_, _PREFIX_, _POSTFIX_) using hashed_##_PREFIX_##string##_POSTFIX_ = basic_hashed_string##_POSTFIX_##<_TYPE_>;
#define DECLARE(_TYPE_, _PREFIX_)    \
    DECLARE_IMPL(_TYPE_, _PREFIX_, ) \
    DECLARE_IMPL(_TYPE_, _PREFIX_, _view)

export namespace nstd::text
{
#ifdef __cpp_lib_char8_t
    DECLARE(char8_t, u8);
#endif
    DECLARE(char, );
    DECLARE(char16_t, w);
    DECLARE(wchar_t, u16);
    DECLARE(wchar_t, u32);

#define OPERATOR_SIMPLE_HEAD template <class Base, template <typename> class Hasher, class Wrapper = hashed_string_wrapper<Base, Hasher>, typename HashType = Wrapper::hash_type>

#define OPERATOR_IMPL(_CHECK_, _RET_, _OP_)                                                                                     \
    template <class Base, template <typename> class Hasher, class Base2, template <typename> class Hasher2>                     \
    constexpr _RET_ operator _OP_(const hashed_string_wrapper<Base, Hasher>& l, const hashed_string_wrapper<Base2, Hasher2>& r) \
    {                                                                                                                           \
        static_assert(std::equality_comparable_with<Base, Base2>, __FUNCSIG__ ": Base classes must be equality comparable!");   \
        static_assert(nstd::same_template<Hasher, Hasher2>(), __FUNCSIG__ ": Hash functions must be have same base!");          \
        return l.hash() _OP_ r.hash();                                                                                          \
    }                                                                                                                           \
    OPERATOR_SIMPLE_HEAD                                                                                                        \
    constexpr _RET_ operator _OP_(const Wrapper& l, const HashType hash)                                                        \
    {                                                                                                                           \
        return l.hash() _OP_ hash;                                                                                              \
    }                                                                                                                           \
    OPERATOR_SIMPLE_HEAD                                                                                                        \
    constexpr _RET_ operator _OP_(const HashType hash, const Wrapper& r)                                                        \
    {                                                                                                                           \
        return hash _OP_ r.hash();                                                                                              \
    }

#define OPERATOR(_OP_) OPERATOR_IMPL(std::equality_comparable_with, bool, _OP_);

    OPERATOR(==);
    OPERATOR(!=);
} // namespace nstd::text
