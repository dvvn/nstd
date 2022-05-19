module;

#include <concepts>

#ifdef __cpp_lib_format
#include <format>
#define _FMT std
#else
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/xchar.h>
#define _FMT fmt
#endif

export module nstd.format;

template <class Lambda, int = (Lambda{}(), 0)>
constexpr bool is_constexpr(Lambda)
{
    return true;
}
constexpr bool is_constexpr(...)
{
    return false;
}

template <typename T>
consteval auto _To_strv_ct(const T& obj) noexcept
{
    std::basic_string_view ret = obj;
    return ret;
}

template <typename C, typename... Args>
auto _Format_args(Args&&... args)
{
    if constexpr (std::same_as<C, char>)
        return _FMT::make_format_args(std::forward<Args>(args)...);
    else if constexpr (std::same_as<C, wchar_t>)
        return _FMT::make_wformat_args(std::forward<Args>(args)...);
}

template <typename C /* , typename Tr = std::char_traits<C> */>
class ct_string
{
    std::basic_string_view<C /* , Tr */> str_;

  public:
    template <typename T>
    consteval ct_string(const T& str) : str_(str)
    {
    }

    auto view() const noexcept
    {
        return str_;
    }
};
template <typename T>
ct_string(const T& obj) -> ct_string<std::remove_cvref_t<decltype(obj[0])>>;

template <typename C /* , typename Tr = std::char_traits<C> */>
class rt_string
{
    std::basic_string_view<C /* , Tr */> str_;

  public:
    template <typename T>
    requires(!std::constructible_from<ct_string<C>, T>) rt_string(const T& str) : str_(str)
    {
    }

    auto view() const noexcept
    {
        return str_;
    }
};
template <typename T>
rt_string(const T& obj) -> rt_string<std::remove_cvref_t<decltype(obj[0])>>;

struct format_impl
{
    template <typename C, typename... Args>
    auto operator()(const ct_string<C>& fmt, Args&&... args) const
    {
        return _FMT::format(fmt.view(), std::forward<Args>(args)...);
    }

    template <typename C, typename... Args>
    auto operator()(const rt_string<C>& fmt, Args&&... args) const
    {
        return _FMT::vformat(fmt.view(), _Format_args<C>(std::forward<Args>(args)...));
    }

    /* template <class OutputIt, typename Fmt, class... Args>
    OutputIt to(OutputIt out, const Fmt& fmt, Args&&... args) const
    {
    }

    template <class OutputIt, typename Fmt, class... Args>
    OutputIt to(OutputIt out, const std::locale& loc, const Fmt& fmt, Args&&... args) const
    {
        if constexpr (is_constexpr([] { return _To_strv_ct<Fmt>(); }))
            return _FMT::format_to(std::move(out), loc, fmt, std::forward<Args>(args)...);
        else
            return _FMT::vformat_to()
    } */
};

export namespace nstd
{
    using _FMT::formatter;
    constexpr format_impl format;
} // namespace nstd
