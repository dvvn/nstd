module;

#include <nstd/core_utils.h>

#include <memory>
#include <optional>

export module nstd.one_instance;

template <typename PtrT>
constexpr size_t _Pointers_count() noexcept
{
    if constexpr (std::is_pointer_v<PtrT>)
        return _Pointers_count<std::remove_pointer_t<PtrT>>() + 1;
    else
        return 0;
}

template <typename T>
auto _Lowest_pointer(T* ptr) noexcept
{
    constexpr auto count = _Pointers_count<T*>();
    if constexpr (count == 1)
        return ptr;
    else if constexpr (count == 2)
        return *ptr;
    else if constexpr (count == 3)
        return **ptr;
    else if constexpr (count == 4)
        return ***ptr;
    else if constexpr (count == 5)
        return ****ptr;
    // otherwise check your code, having so many pointers is not normal
}

template <typename T>
bool _Nullptr_check(T* ptr) noexcept
{
    if (ptr == nullptr)
        return true;

    constexpr auto limit = _Pointers_count<T>();
    if constexpr (limit > 0)
    {
        auto check = (void**)*ptr;
        auto num = limit;
        for (;;)
        {
            if (check == nullptr)
                return true;
            if (--num == 0)
                break;
            check = (void**)*check;
        }
    }

    return false;
}

template <typename T>
class pointer_wrapper
{
    T ptr_;

  public:
    pointer_wrapper(T ptr)
        : ptr_(ptr)
    {
    }

    T operator->() const noexcept
    {
        return ptr_;
    }

    auto& operator*() const noexcept
    {
        return *ptr_;
    }
};

template <typename T>
class pointer_wrapper<T**>
{
    T** ptr_;

    bool is_null() const noexcept
    {
        return _Nullptr_check(ptr_);
    }

    auto get() const noexcept
    {
        return _Lowest_pointer(ptr_);
    }

  public:
    pointer_wrapper(T** ptr)
        : ptr_(ptr)
    {
    }

    auto operator->() const noexcept
    {
        return get();
    }

    auto& operator*() const noexcept
    {
        return *get();
    }

    /*  bool operator==(nullptr_t) const noexcept
     {
         return is_null();
     }

     bool operator!=(nullptr_t) const noexcept
     {
         return !is_null();
     }

    bool operator!() const noexcept
     {
         return is_null();
     } */

    explicit operator bool() const noexcept
    {
        return !is_null();
    }
};

//---

template <typename T>
struct one_instance_getter
{
    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

    template <size_t Instance, typename... Args>
    one_instance_getter(const std::in_place_index_t<Instance>, Args&&... args)
        : item_(std::forward<Args>(args)...)
    {
    }

    reference ref() noexcept
    {
        return item_;
    }

    pointer ptr() noexcept
    {
        return std::addressof(item_);
    }

  private:
    value_type item_;
};

template <typename T, typename D>
class one_instance_getter<std::unique_ptr<T, D>>
{
  public:
    using value_type = std::unique_ptr<T, D>;

    using element_type = T;
    using deleter_type = D;

    using pointer = value_type::pointer;
    using reference = std::remove_pointer_t<pointer>;

    template <size_t Instance, typename... Args>
    one_instance_getter(const std::in_place_index_t<Instance>, Args&&... args)
        : item_(std::make_unique<T>(std::forward<Args>(args)...))
    {
    }

    reference ref() noexcept
    {
        return *item_;
    }

    pointer ptr() noexcept
    {
        return item_.get();
    }

  private:
    value_type item_;
};

template <typename T>
class one_instance_getter<T*>
{
  public:
    using element_type = T*;
    using value_type = nstd::remove_all_pointers_t<element_type>;
    using reference = value_type&;
    using real_pointer = value_type*;
    using pointer = std::conditional_t<std::is_pointer_v<T>, pointer_wrapper<element_type>, real_pointer>;

    template <size_t Instance>
    one_instance_getter(const std::in_place_index_t<Instance>);

    reference ref() const noexcept
    {
        return *_Lowest_pointer(item_);
    }

    pointer ptr() const noexcept
    {
        return item_;
    }

  private:
    element_type item_;
};

constexpr uint8_t time_offsets[] = {0, 3, 6};

template <typename T, size_t Instance = 0>
class one_instance
{
    using getter_type = one_instance_getter<T>;

    static auto& _Buff() noexcept
    {
        static std::optional<getter_type> buff;
        return buff;
    }

    template <typename... Args>
    static auto& _Construct(Args&&... args)
    {
        return _Buff().emplace(std::in_place_index<Instance>, std::forward<Args>(args)...);
    }

    static auto& _Get() noexcept
    {
        static const auto once = [] {
            if (!initialized())
                _Construct();
            constexpr size_t src = time_offsets[Instance % 3];
            return __TIME__[src] ^ __TIME__[7]; // XX:XX:XX 01 2 34 5 67
        }();

        return *_Buff();
    }

  public:
    constexpr one_instance() = default;
    constexpr one_instance(const one_instance& other) = delete;
    constexpr one_instance& operator=(const one_instance& other) = delete;
    constexpr one_instance(one_instance&& other) noexcept = delete;
    constexpr one_instance& operator=(one_instance&& other) noexcept = delete;

    static bool initialized() noexcept
    {
        return _Buff().has_value();
    }

    static auto& get() noexcept
    {
        return _Get().ref();
    }

    static auto get_ptr() noexcept
    {
        return _Get().ptr();
    }

    template <typename... Args>
    static auto& construct(Args&&... args)
    {
        _Construct(std::forward<Args>(args)...);
        return get();
    }
};

template <typename T, size_t Instance>
class instance_of_t
{
    using _Base = one_instance<T, Instance>;

  public:
    /*constexpr instance_of_t( ) = default;
    constexpr instance_of_t(const instance_of_t& other) = delete;
    constexpr instance_of_t& operator=(const instance_of_t& other) = delete;
    constexpr instance_of_t(instance_of_t&& other) noexcept = delete;
    constexpr instance_of_t& operator=(instance_of_t&& other) noexcept = delete;*/

    bool initialized() const noexcept
    {
        return _Base::initialized();
    }

    auto& operator*() const noexcept
    {
        return _Base::get();
    }

    auto operator->() const noexcept
    {
        return _Base::get_ptr();
    }

    auto operator&() const noexcept
    {
        return _Base::get_ptr();
    }

    template <typename... Args>
    auto& construct(Args&&... args) const noexcept
    {
        return _Base::construct(std::forward<Args>(args)...);
    }

    template <std::same_as<size_t> T> // fake explicit
    consteval operator T() const noexcept
    {
        return Instance;
    }
};

export namespace nstd
{
    using ::one_instance;
    using ::one_instance_getter;

    template <typename T, size_t Instance = 0>
    constexpr instance_of_t<std::conditional_t<std::is_abstract_v<T>, T*, T>, Instance> instance_of;
} // namespace nstd
