module;

#include <nstd/type_traits.h>
#include <memory>

export module nstd.one_instance;

template <typename Last = void, typename T>
auto deref_ptr(T ptr) noexcept
{
	if constexpr(!std::is_pointer_v<std::remove_pointer_t<T>> || std::same_as<Last, T>)
		return ptr;
	else
		return deref_ptr<Last>(*ptr);
}

template<typename T>
bool nullptr_check(T ptr) noexcept
{
	if(ptr == nullptr)
		return true;

	if constexpr(!std::is_pointer_v<std::remove_pointer_t<T>>)
		return false;
	else
		return nullptr_check(*ptr);
}

template <typename T>
class pointer_wrapper
{
	T ptr_;

public:

	pointer_wrapper(T ptr)
		:ptr_(ptr)
	{
	}

	T operator->( ) const noexcept
	{
		return ptr_;
	}

	auto& operator*( ) const noexcept
	{
		return *ptr_;
	}
};

template <typename T>
class pointer_wrapper<T**>
{
	T** ptr_;

	bool _Is_null( ) const noexcept
	{
		return nullptr_check(ptr_);
	}

public:

	pointer_wrapper(T** ptr)
		:ptr_(ptr)
	{
	}

	auto operator->( ) const noexcept
	{
		return deref_ptr(ptr_);
	}

	auto& operator*( ) const noexcept
	{
		return *deref_ptr(ptr_);
	}

	bool operator==(nullptr_t) const noexcept
	{
		return _Is_null( );
	}

	bool operator!=(nullptr_t) const noexcept
	{
		return !_Is_null( );
	}

	operator bool( ) const noexcept
	{
		return !_Is_null( );
	}

	bool operator!( ) const noexcept
	{
		return _Is_null( );
	}
};

template <typename T>
void recreate(T& ref) noexcept
{
	auto ptr = std::addressof(ref);
	std::destroy_at(ptr);
	std::construct_at(ptr);
}

template<size_t Instance>
auto _Init_helper(bool& init_tag)
{
	init_tag = true;
	return std::in_place_index<Instance>;
}

export namespace nstd
{
	template <typename T>
	struct one_instance_getter
	{
		using value_type = T;
		using reference = value_type&;
		using pointer = value_type*;

		template<size_t Instance>
		one_instance_getter(const std::in_place_index_t<Instance>)
			:item_( )
		{
		}

		reference ref( ) noexcept
		{
			return item_;
		}

		pointer ptr( ) noexcept
		{
			return std::addressof(item_);
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

		template<size_t Instance>
		one_instance_getter(const std::in_place_index_t<Instance>);

		reference ref( ) const noexcept
		{
			return *deref_ptr(item_);
		}

		pointer ptr( ) const noexcept
		{
			return item_;
		}

	private:
		element_type item_;
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

		template<size_t Instance>
		one_instance_getter(const std::in_place_index_t<Instance>)
			:item_(std::make_unique<T>( ))
		{
		}

		reference ref( ) noexcept
		{
			return *item_;
		}

		pointer ptr( ) noexcept
		{
			return item_.get( );
		}

	private:
		value_type item_;
	};

	template <typename T, size_t Instance = 0>
	class one_instance
	{
	public:
		using getter_type = one_instance_getter<T>;

	private:
		static auto& _Get( ) noexcept
		{
			static getter_type g = _Init_helper<Instance>(_Initialized( ));
			return g;
		}

		static bool& _Initialized( ) noexcept
		{
			static bool val = false;
			return val;
		}

	public:
		constexpr one_instance( ) = default;
		constexpr one_instance(const one_instance& other) = delete;
		constexpr one_instance& operator=(const one_instance& other) = delete;
		constexpr one_instance(one_instance&& other) noexcept = delete;
		constexpr one_instance& operator=(one_instance&& other) noexcept = delete;

		static bool initialized( ) noexcept
		{
			return _Initialized( );
		}

		static auto& get( ) noexcept
		{
			return _Get( ).ref( );
		}

		static auto get_ptr( ) noexcept
		{
			return _Get( ).ptr( );
		}

		[[deprecated]]
		static void _Recreate( ) noexcept
		{
			recreate(_Get( ));
		}
	};

	template <typename T, size_t Instance = 0, class Base = one_instance<std::conditional_t<std::is_abstract_v<T>, T*, T>, Instance>>
	class instance_of_t
	{
	public:
		/*constexpr instance_of_t( ) = default;
		constexpr instance_of_t(const instance_of_t& other) = delete;
		constexpr instance_of_t& operator=(const instance_of_t& other) = delete;
		constexpr instance_of_t(instance_of_t&& other) noexcept = delete;
		constexpr instance_of_t& operator=(instance_of_t&& other) noexcept = delete;*/

		bool initialized( ) const noexcept
		{
			return Base::initialized( );
		}

		auto& operator*( ) const noexcept
		{
			return Base::get( );
		}

		auto operator->( ) const noexcept
		{
			return Base::get_ptr( );
		}

		auto operator&( ) const noexcept
		{
			return Base::get_ptr( );
		}
	};

	template <typename T, size_t Instance = 0>
	constexpr instance_of_t<T, Instance> instance_of;
}
