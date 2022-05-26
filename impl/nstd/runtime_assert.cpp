#include <nstd/runtime_assert_impl.h>

#ifdef _DEBUG
#include <stdexcept>
#endif
#include <algorithm>
#include <mutex>
#include <variant>
#include <vector>
#undef NDEBUG
#include <cassert>

import nstd.one_instance;

template <typename C, typename Arg1, typename... Args>
static auto _Build_string(const Arg1& arg, const Args&... args)
{
    using char_t = std::remove_cvref_t<decltype(arg[0])>;
    const std::basic_string_view<char_t> arg_sv = arg;
    if constexpr (sizeof...(Args) == 0 && std::is_same_v<char_t, C>)
    {
        return arg_sv;
    }
    else
    {
        return std::apply(
            []<typename... S>(const S&... strings) {
                std::basic_string<C> buff;
                buff.reserve((strings.size() + ...));
                (buff.append(strings.begin(), strings.end()), ...);
                return buff;
            },
            std::make_tuple(arg_sv, std::basic_string_view(args)...));
    }
}

template <typename C>
class msg_packed
{
  public:
    using string_type = std::basic_string<C>;
    using string_view_type = std::basic_string_view<C>;
    using pointer_type = const C*;

    msg_packed() = default;

    msg_packed(string_type&& str)
        : data_(std::move(str))
    {
    }

    msg_packed(pointer_type ptr)
        : data_(ptr)
    {
    }

    template <typename T>
    msg_packed(const T* ptr)
    {
        static_assert(sizeof(T) < sizeof(C));
        const std::basic_string_view tmp = ptr;
        data_.emplace<string_type>(tmp.begin(), tmp.end());
    }

    operator pointer_type() const
    {
        return std::visit(
            []<typename T>(const T& obj) {
                if constexpr (std::is_class_v<T>)
                    return obj.data();
                else
                    return obj;
            },
            data_);
    }

  private:
    std::variant<string_type, string_view_type, pointer_type> data_;
};

template <typename C>
msg_packed(const C*) -> msg_packed<C>;

template <typename C>
static msg_packed<C> _Assert_msg(const char* expression, const char* message)
{
    if (!expression)
        return message;
    if (!message)
        return expression;
    return _Build_string<C>(message, "( ", expression, ")");
}

static void _Assert(const char* expression, const char* message, const std::source_location& location)
{
#if defined(_MSC_VER)
    _wassert(_Assert_msg<wchar_t>(expression, message), msg_packed<wchar_t>(location.file_name()), location.line());
#elif defined(__GNUC__)
    __assert_fail(_Assert_msg<char>(expression, message), location.file_name(), location.line(), location.function_name());
#else
    TODO
#endif
}

using nstd::rt_assert_handler;
struct rt_assert_handler_default final : rt_assert_handler
{
    void handle(const char* expression, const char* message, const std::source_location& location) override
    {
        _Assert(expression, message, location);
    }

    void handle(const char* message, const std::source_location& location) override
    {
        _Assert(nullptr, message, location);
    }
};

class rt_assert_data
{
    std::mutex mtx_;
    std::vector<rt_assert_handler*> storage_;

  public:
    template <bool Lock = true>
    void add(rt_assert_handler* const handler)
    {
        if constexpr (Lock)
            mtx_.lock();

#ifdef _DEBUG
        if (!storage_.empty())
        {
            for (const auto& el : storage_)
            {
                if (el == handler)
                    throw std::logic_error("Handler already added!");
            }
        }
#endif
        storage_.push_back(handler);

        if constexpr (Lock)
            mtx_.unlock();
    }

    void remove(rt_assert_handler* const handler)
    {
        const auto lock = std::scoped_lock(mtx_);
        const auto end = storage_.end();
        for (auto itr = storage_.begin(); itr != end; ++itr)
        {
            if (handler == *itr)
            {
                storage_.erase(itr);
                break;
            }
        }
    }

    template <typename... Args>
    void handle(const Args&... args)
    {
        const auto lock = std::scoped_lock(mtx_);
        for (const auto& el : storage_)
            el->handle(args...);
    }

    rt_assert_data() = default;
};

namespace nstd
{
    constexpr auto _Rt = instance_of<rt_assert_data>;

    void _Rt_assert_add(rt_assert_handler* const handler)
    {
        _Rt->add(handler);
    }

    void _Rt_assert_remove(rt_assert_handler* const handler)
    {
        _Rt->remove(handler);
    }

    void _Rt_assert_invoke(const char* expression, const char* message, const std::source_location& location)
    {
        _Rt->handle(expression, message, location);
        std::terminate();
    }
} // namespace nstd
