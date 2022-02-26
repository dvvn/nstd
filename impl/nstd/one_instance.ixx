module;

#include <memory>

export module nstd.one_instance;

export namespace nstd
{
	template <typename T>
	class one_instance_default_getter
	{
		T item_;

	public:
		one_instance_default_getter( )
			:item_( )
		{
		}

		operator T& ()
		{
			return item_;
		}

		operator const T& () const
		{
			return item_;
		}
	};

	/*template <typename T>
	class one_instance_default_getter<std::shared_ptr<T>>
	{
		using value_type = std::shared_ptr<T>;
		value_type item_;

	public:
		one_instance_default_getter( )
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

	template <typename T, size_t Index = 0, class Getter = one_instance_default_getter<T>>
	class one_instance
	{
		static Getter& _Get( )
		{
			static_assert(std::is_default_constructible_v<Getter>, __FUNCSIG__": getter must be default constructible!");
			static Getter getter;
			return getter;
		}

	public:
		constexpr one_instance( ) = default;
		constexpr one_instance(const one_instance& other) = delete;
		constexpr one_instance& operator=(const one_instance& other) = delete;
		constexpr one_instance(one_instance&& other) noexcept = delete;
		constexpr one_instance& operator=(one_instance&& other) noexcept = delete;

		/*using value_type = T;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;*/

		using element_type = T;

		static T& get( )
		{
			return _Get( );
		}

		static T* get_ptr( )
		{
			return std::addressof(get( ));
		}

		static void _Reload( )
		{
			auto ptr = std::addressof(_Get( ));
			std::destroy_at(ptr);
			std::construct_at(ptr);
		}
	};
}
