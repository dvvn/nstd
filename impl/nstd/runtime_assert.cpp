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
            []<typename... S>(const S&... strings) noexcept {
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

    msg_packed(string_type&& str) : data_(std::move(str))
    {
    }
    msg_packed(pointer_type ptr) : data_(ptr)
    {
    }

    template <typename T>
    msg_packed(const T* ptr)
    {
        static_assert(sizeof(T) < sizeof(C));
        const std::basic_string_view tmp = ptr;
        data_.emplace<string_type>(tmp.begin(), tmp.end());
    }

    operator pointer_type() const noexcept
    {
        return std::visit(
            []<typename T>(const T& obj) noexcept {
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
static msg_packed<C> _Assert_msg(const char* expression, const char* message) noexcept
{
    if (!expression)
        return message;
    if (!message)
        return expression;
    return _Build_string<C>(message, "( ", expression, ")");
}

static void _Assert(const char* expression, const char* message, const std::source_location& location) noexcept
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
    void handle(const char* expression, const char* message, const std::source_location& location) noexcept override
    {
        _Assert(expression, message, location);
    }

    void handle(const char* message, const std::source_location& location) noexcept override
    {
        _Assert(nullptr, message, location);
    }
};

class rt_assert_entry
{
    using value_type = std::variant<rt_assert_handler::unique, rt_assert_handler::shared, rt_assert_handler::raw>;
    value_type data_;

  public:
    rt_assert_entry(rt_assert_handler::unique&& data) : data_(std::move(data))
    {
    }

    rt_assert_entry(const rt_assert_handler::shared& data) : data_(data)
    {
    }

    rt_assert_entry(rt_assert_handler::raw data) : data_(data)
    {
    }

    rt_assert_entry(rt_assert_entry&& other) noexcept
    {
        *this = std::move(other);
    }

    rt_assert_entry& operator=(rt_assert_entry&& other) noexcept
    {
        using std::swap;
        swap(data_, other.data_);

        return *this;
    }

    rt_assert_handler* get() const noexcept
    {
        return std::visit(
            []<typename T>(const T& obj) noexcept {
                if constexpr (std::is_class_v<T>)
                    return obj.get();
                else
                    return obj;
            },
            data_);
    }

    rt_assert_handler* operator->() const noexcept
    {
        return get();
    }
};

class rt_assert_data
{
    std::mutex mtx_;
    std::vector<rt_assert_entry> storage_;

  public:
    template <bool Lock = true>
    void add(rt_assert_entry&& entry) runtime_assert_noexcept
    {
        if constexpr (Lock)
            mtx_.lock();

#ifdef _DEBUG
        if (!storage_.empty())
        {
            const size_t id = entry->id();
            for (const auto& el : storage_)
            {
                if (el->id() == id)
                    throw std::logic_error("Handler with given id already exists!");
            }
        }
#endif
        storage_.push_back(std::move(entry));

        if constexpr (Lock)
            mtx_.unlock();
    }

    void remove(const size_t id) noexcept
    {
        const auto lock = std::scoped_lock(mtx_);
        const auto end = storage_.end();
        for (auto itr = storage_.begin(); itr != end; ++itr)
        {
            if (id == (*itr)->id())
            {
                storage_.erase(itr);
                break;
            }
        }
    }

    template <typename... Args>
    void handle(const Args&... args) noexcept
    {
        const auto lock = std::scoped_lock(mtx_);
        for (const auto& el : storage_)
            el->handle(args...);
    }

    rt_assert_data()
    {
        rt_assert_handler::unique ptr = std::make_unique<rt_assert_handler_default>();
        this->add<false>(std::move(ptr));
    }
};

constexpr auto _Rt = nstd::instance_of<rt_assert_data>;

void nstd::_Rt_assert_add(rt_assert_handler::unique&& handler) runtime_assert_noexcept
{
    _Rt->add(std::move(handler));
}

void nstd::_Rt_assert_add(const rt_assert_handler::shared& handler) runtime_assert_noexcept
{
    _Rt->add(handler);
}

void nstd::_Rt_assert_add(rt_assert_handler::raw handler) runtime_assert_noexcept
{
    _Rt->add(handler);
}

void nstd::_Rt_assert_remove(const size_t id) noexcept
{
    _Rt->remove(id);
}

void nstd::_Rt_assert_handle(bool expression_result, const char* expression, const char* message, const std::source_location& location) noexcept
{
    if (expression_result)
        return;
    _Rt->handle(expression, message, location);
}

void nstd::_Rt_assert_handle(const char* message, [[maybe_unused]] const char* unused1, [[maybe_unused]] const char* unused2, const std::source_location& location) noexcept
{
    _Rt->handle(message, location);
    std::terminate();
}
