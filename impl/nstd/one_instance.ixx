module;

#include <nstd/type_traits.h>
#include <memory>

export module nstd.one_instance;

template <typename Last = void, typename T>
auto deref_ptr(T ptr) noexcept
{
	if constexpr (!std::is_pointer_v<std::remove_pointer_t<T>> || std::same_as<Last, T>)
		return ptr;
	else
		return deref_ptr<Last>(*ptr);
}

template<typename T>
bool nullptr_check(T ptr) noexcept
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
		value_type _Construct( ) const noexcept { return {}; }
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
		element_type _Construct( ) const noexcept;
	};

	template <typename T>
	class one_instance_getter<std::unique_ptr<T>>
	{
	public:
		using element_type = std::unique_ptr<T>;
		using value_type = T;
		using reference = value_type&;
		using pointer = value_type*;

		one_instance_getter( )
			:item_(_Construct( ))
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
		element_type item_;
		element_type _Construct( ) const noexcept { return std::make_unique<T>( ); }
	};

	/*template <typename T>
	one_instance_getter(T)->one_instance_getter<T>;*/

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
		static auto& _Get( ) noexcept
		{
			static auto g = _Init( );
			return g;
		}

		static bool& _Initialized( ) noexcept
		{
			static bool val = false;
			return val;
		}

		static one_instance_getter<T> _Init( ) noexcept
		{
			_Initialized( ) = true;
			return {};
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

		static void _Recreate( ) noexcept
		{
			recreate(_Get( ));
		}
	};

	template <typename T, size_t Instance = 0>
	class one_instance_obj final
	{
		static auto& _Get( ) noexcept
		{
			static auto g = _Init( );
			return g;
		}

		static bool& _Initialized( ) noexcept
		{
			static bool val = false;
			return val;
		}

		static one_instance_getter<T> _Init( ) noexcept
		{
			_Initialized( ) = true;
			return {};
		}

	public:
		constexpr one_instance_obj( ) = default;
		constexpr one_instance_obj(const one_instance_obj& other) = delete;
		constexpr one_instance_obj& operator=(const one_instance_obj& other) = delete;
		constexpr one_instance_obj(one_instance_obj&& other) noexcept = delete;
		constexpr one_instance_obj& operator=(one_instance_obj&& other) noexcept = delete;

		bool initialized( ) const noexcept
		{
			return _Initialized( );
		}

		auto& operator*( ) const noexcept
		{
			return _Get( ).ref( );
		}

		auto operator->( ) const noexcept
		{
			return _Get( ).ptr( );
		}

		void _Recreate( ) const noexcept
		{
			recreate(_Get( ));
		}
	};

	template <typename T, size_t Instance = 0>
	decltype(auto) get_instance( ) noexcept
	{
		constexpr one_instance_obj<T, Instance> obj;
		if constexpr (std::is_pointer_v<T>)
			return obj.operator->( );
		else
			return *obj;
	}
}
