module;

#include <nstd/type_traits.h>
#include <memory>

export module nstd.one_instance;

template <typename Last = void, typename T>
auto deref_ptr(T ptr)
{
	if constexpr (!std::is_pointer_v<std::remove_pointer_t<T>> || std::same_as<Last, T>)
		return ptr;
	else
		return deref_ptr<Last>(*ptr);
}

template<typename T>
bool nullptr_check(T ptr)
{
	if (ptr == nullptr)
		return true;

	if constexpr (!std::is_pointer_v<std::remove_pointer_t<T>>)
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

	T operator->( )const
	{
		return ptr_;
	}

	auto& operator*( )const
	{
		return *ptr_;
	}
};

template <typename T>
class pointer_wrapper<T**>
{
	T** ptr_;

	bool _Is_null( )const
	{
		return nullptr_check(ptr_);
	}

public:

	pointer_wrapper(T** ptr)
		:ptr_(ptr)
	{
	}

	auto operator->( )const
	{
		return deref_ptr(ptr_);
	}

	auto& operator*( )const
	{
		return *deref_ptr(ptr_);
	}

	bool operator==(nullptr_t)const
	{
		return _Is_null( );
	}

	bool operator!=(nullptr_t)const
	{
		return !_Is_null( );
	}

	operator bool( )const
	{
		return !_Is_null( );
	}

	bool operator!( )const
	{
		return _Is_null( );
	}
};

export namespace nstd
{
	template <typename T>
	struct one_instance_getter
	{
		using value_type = T;
		using reference = value_type&;
		using pointer = value_type*;

		one_instance_getter( )
			:item_(_Construct( ))
		{
		}

		reference ref( )
		{
			return item_;
		}

		pointer ptr( )
		{
			return std::addressof(item_);
		}

	private:
		value_type item_;
		value_type _Construct( ) const { return {}; }
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

		one_instance_getter( )
			:item_(_Construct( ))
		{
		}

		reference ref( )const
		{
			return *deref_ptr(item_);
		}

		pointer ptr( )const
		{
			return item_;
		}

	private:
		element_type item_;
		element_type _Construct( ) const;
	};

	template <typename T>
	one_instance_getter(T)->one_instance_getter<T>;

	/*template <typename T>
class one_instance_getter<std::shared_ptr<T>>
{
	using value_type = std::shared_ptr<T>;
	value_type item_;

public:
	one_instance_getter( )
		:item_(std::make_shared<T>( ))
	{
	}

	operator T& () const
	{
		return *item_;
	}

	operator value_type& ()
	{
		return item_;
	}

	operator const value_type& () const
	{
		return item_;
	}
};*/

	template <typename T, size_t Instance = 0>
	class one_instance
	{
		static auto& _Get( )
		{
			static one_instance_getter<T> g;
			return g;
		}

	public:
		constexpr one_instance( ) = default;
		constexpr one_instance(const one_instance& other) = delete;
		constexpr one_instance& operator=(const one_instance& other) = delete;
		constexpr one_instance(one_instance&& other) noexcept = delete;
		constexpr one_instance& operator=(one_instance&& other) noexcept = delete;

		static auto& get( )
		{
			return _Get( ).ref( );
		}

		static auto get_ptr( )
		{
			return _Get( ).ptr( );
		}

		static void _Reload( )
		{
			auto ptr = std::addressof(_Get( ));
			std::destroy_at(ptr);
			std::construct_at(ptr);
		}
	};
}
